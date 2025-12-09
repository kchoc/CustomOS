#include "kernel/filesystem/sockfs.h"
#include "kernel/filesystem/vfs.h"
#include "kernel/filesystem/file.h"
#include "kernel/socket/socket.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"
#include "types/string.h"

/* Forward declarations */
static dentry_t* sockfs_root = NULL;
static unsigned long next_socket_ino = 1;

/* Message buffer structure for queue */
typedef struct sock_msg {
    list_node_t node;
    void* data;
    size_t len;
    file_t* sender;  // For connection tracking
} sock_msg_t;

/* ================
   SOCKFS OPERATIONS
   ================ */

static ssize_t sockfs_read(file_t* file, char* __user buf, size_t count, loff_t *offset);
static ssize_t sockfs_write(file_t* file, const char* __user buf, size_t count, loff_t *offset);
static int sockfs_open(inode_t* inode, file_t* file);
static int sockfs_release(inode_t* inode, file_t* file);

static int sockfs_socket_listen(dentry_t* socket_node, int backlog);
static file_t* sockfs_socket_accept(dentry_t* socket_node, int flags);
static int sockfs_socket_connect(dentry_t* socket_node, file_t* file, int flags);
static ssize_t sockfs_socket_sendmsg(file_t* file, const void* __user buf, size_t len, int flags);
static ssize_t sockfs_socket_recvmsg(file_t* file, void* __user buf, size_t len, int flags);
static int sockfs_socket_release(dentry_t* socket_node, file_t* file);

/* File operations for socket files */
static const file_ops_t sockfs_file_ops = {
    .llseek = NULL,
    .read = sockfs_read,
    .write = sockfs_write,
    .open = sockfs_open,
    .release = sockfs_release,
    .iterate_shared = NULL,
};

/* Socket operations for socket files */
static const socket_ops_t sockfs_socket_ops = {
    .listen = sockfs_socket_listen,
    .accept = sockfs_socket_accept,
    .connect = sockfs_socket_connect,
    .sendmsg = sockfs_socket_sendmsg,
    .recvmsg = sockfs_socket_recvmsg,
    .release = sockfs_socket_release,
};

/* Inode operations for sockfs directory */
static const inode_ops_t sockfs_dir_inode_ops = {
    .lookup = NULL,
    .create = NULL,
    .link = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .rename = NULL,
};

/* Superblock operations */
static const sb_ops_t sockfs_sb_ops = {
    .alloc_inode = NULL,
    .destroy_inode = NULL,
    .write_inode = NULL,
    .drop_inode = NULL,
    .put_super = NULL,
    .statfs = NULL,
};

/* ================
   HELPER FUNCTIONS
   ================ */

/**
 * Find a socket dentry by path within sockfs
 */
static dentry_t* sockfs_find_socket(const char* name) {
    if (!sockfs_root || !name) return NULL;
    
    // Search through children of sockfs root
    list_node_t* node = sockfs_root->children.head;
    while (node) {
        dentry_t* child = (dentry_t*)((uint8_t*)node - offsetof(dentry_t, node));
        if (child->d_name && strcmp(child->d_name, name) == 0) {
            return child;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * Create a new socket inode
 */
static inode_t* sockfs_create_inode(super_block_t* sb, sock_type_t type) {
    inode_t* inode = alloc_inode(next_socket_ino++, UMODE_IFSOCK | 0666, 0,
                                  NULL, &sockfs_file_ops);
    if (!inode) return NULL;
    
    inode->i_sb = sb;
    
    // Allocate and initialize socket structure
    socket_t* sock = kmalloc(sizeof(socket_t));
    if (!sock) {
        kfree(inode);
        return NULL;
    }
    
    memset(sock, 0, sizeof(socket_t));
    sock->type = type;
    sock->state = SOCK_STATE_CLOSED;
    sock->backlog = 0;
    list_init(&sock->accept_queue, false);
    list_init(&sock->peer_queue, false);
    list_init(&sock->read_queue, false);
    list_init(&sock->write_queue, false);
    sock->private = NULL;
    
    inode->private = sock;
    
    return inode;
}

/* ================
   FILE OPERATIONS
   ================ */

static ssize_t sockfs_read(file_t* file, char* __user buf, size_t count, loff_t *offset) {
    if (!file || !file->f_inode || !file->f_inode->private || !buf) return -1;
    
    socket_t* sock = (socket_t*)file->f_inode->private;
    
    // Check if there's data in the read queue
    if (!sock->read_queue.head) {
        return 0; // No data available (non-blocking)
    }
    
    // Dequeue the first message
    list_node_t* node = list_pop_head(&sock->read_queue);
    if (!node) return 0;
    
    sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
    
    // Copy data to user buffer
    size_t to_copy = msg->len < count ? msg->len : count;
    memcpy(buf, msg->data, to_copy);
    
    // Free the message
    kfree(msg->data);
    kfree(msg);
    
    return to_copy;
}

static ssize_t sockfs_write(file_t* file, const char* __user buf, size_t count, loff_t *offset) {
    if (!file || !file->f_inode || !file->f_inode->private || !buf) return -1;
    
    socket_t* sock = (socket_t*)file->f_inode->private;
    
    if (sock->state != SOCK_STATE_CONNECTED && sock->state != SOCK_STATE_OPEN) {
        return -1; // Socket not ready for writing
    }
    
    // Allocate message buffer
    sock_msg_t* msg = kmalloc(sizeof(sock_msg_t));
    if (!msg) return -1;
    
    msg->data = kmalloc(count);
    if (!msg->data) {
        kfree(msg);
        return -1;
    }
    
    memcpy(msg->data, buf, count);
    msg->len = count;
    msg->sender = file;
    
    // Enqueue to write queue
    list_push_tail(&sock->write_queue, &msg->node);
    
    return count;
}

static int sockfs_open(inode_t* inode, file_t* file) {
    if (!inode || !inode->private) return -1;
    
    socket_t* sock = (socket_t*)inode->private;
    
    // Mark socket as open
    if (sock->state == SOCK_STATE_CLOSED) {
        sock->state = SOCK_STATE_OPEN;
    }
    
    return 0;
}

static int sockfs_release(inode_t* inode, file_t* file) {
    if (!inode || !inode->private) return -1;
    
    socket_t* sock = (socket_t*)inode->private;
    
    // Close the socket
    sock->state = SOCK_STATE_CLOSED;
    
    return 0;
}

/* ================
   SOCKET OPERATIONS
   ================ */

static int sockfs_socket_listen(dentry_t* socket_node, int backlog) {
    if (!socket_node || !socket_node->d_inode || !socket_node->d_inode->private)
        return -1;
    
    socket_t* sock = (socket_t*)socket_node->d_inode->private;
    
    if (sock->type != SOCK_TYPE_STREAM) {
        printf("sockfs: Only stream sockets can listen\n");
        return -1;
    }
    
    sock->state = SOCK_STATE_LISTENING;
    sock->backlog = backlog;
    
    return 0;
}

static file_t* sockfs_socket_accept(dentry_t* socket_node, int flags) {
    if (!socket_node || !socket_node->d_inode || !socket_node->d_inode->private)
        return NULL;
    
    socket_t* sock = (socket_t*)socket_node->d_inode->private;
    
    if (sock->state != SOCK_STATE_LISTENING) {
        printf("sockfs: Socket is not listening\n");
        return NULL;
    }
    
    // Check accept queue for pending connections
    if (!sock->accept_queue.head) {
        return NULL; // No pending connections (non-blocking)
    }
    
    // Dequeue the first connection
    list_node_t* node = list_pop_head(&sock->accept_queue);
    if (!node) return NULL;
    
    sock_msg_t* conn_msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
    file_t* client_file = conn_msg->sender;
    
    kfree(conn_msg);
    
    return client_file;
}

static int sockfs_socket_connect(dentry_t* socket_node, file_t* file, int flags) {
    if (!socket_node || !socket_node->d_inode || !socket_node->d_inode->private)
        return -1;
    
    socket_t* sock = (socket_t*)socket_node->d_inode->private;
    
    if (sock->state == SOCK_STATE_LISTENING) {
        // Add this connection to accept queue
        sock_msg_t* conn_msg = kmalloc(sizeof(sock_msg_t));
        if (!conn_msg) return -1;
        
        conn_msg->sender = file;
        conn_msg->data = NULL;
        conn_msg->len = 0;
        
        list_push_tail(&sock->accept_queue, &conn_msg->node);
        
        // Store peer socket for bi-directional communication
        if (file && file->f_inode && file->f_inode->private) {
            socket_t* peer_sock = (socket_t*)file->f_inode->private;
            peer_sock->state = SOCK_STATE_CONNECTED;
            
            // Link the sockets together via peer_queue
            sock_msg_t* peer_link = kmalloc(sizeof(sock_msg_t));
            if (peer_link) {
                peer_link->sender = file;
                peer_link->data = sock;
                peer_link->len = 0;
                list_push_tail(&peer_sock->peer_queue, &peer_link->node);
            }
        }
        
        return 0;
    } else if (sock->state == SOCK_STATE_OPEN) {
        sock->state = SOCK_STATE_CONNECTED;
        return 0;
    }
    
    printf("sockfs: Cannot connect to socket in state %d\n", sock->state);
    return -1;
}

static ssize_t sockfs_socket_sendmsg(file_t* file, const void* __user buf, size_t len, int flags) {
    if (!file || !file->f_inode || !file->f_inode->private || !buf) return -1;
    
    socket_t* sock = (socket_t*)file->f_inode->private;
    
    if (sock->state != SOCK_STATE_CONNECTED) {
        printf("sockfs: Socket not connected\n");
        return -1;
    }
    
    // Find peer socket from peer_queue
    socket_t* peer_sock = NULL;
    if (sock->peer_queue.head) {
        sock_msg_t* peer_link = (sock_msg_t*)((uint8_t*)sock->peer_queue.head - offsetof(sock_msg_t, node));
        peer_sock = (socket_t*)peer_link->data;
    }
    
    if (!peer_sock) {
        // No peer, enqueue to own write_queue
        sock_msg_t* msg = kmalloc(sizeof(sock_msg_t));
        if (!msg) return -1;
        
        msg->data = kmalloc(len);
        if (!msg->data) {
            kfree(msg);
            return -1;
        }
        
        memcpy(msg->data, buf, len);
        msg->len = len;
        msg->sender = file;
        
        list_push_tail(&sock->write_queue, &msg->node);
    } else {
        // Enqueue directly to peer's read queue
        sock_msg_t* msg = kmalloc(sizeof(sock_msg_t));
        if (!msg) return -1;
        
        msg->data = kmalloc(len);
        if (!msg->data) {
            kfree(msg);
            return -1;
        }
        
        memcpy(msg->data, buf, len);
        msg->len = len;
        msg->sender = file;
        
        list_push_tail(&peer_sock->read_queue, &msg->node);
    }
    
    return len;
}

static ssize_t sockfs_socket_recvmsg(file_t* file, void* __user buf, size_t len, int flags) {
    if (!file || !file->f_inode || !file->f_inode->private || !buf) return -1;
    
    socket_t* sock = (socket_t*)file->f_inode->private;
    
    if (sock->state != SOCK_STATE_CONNECTED && sock->state != SOCK_STATE_OPEN) {
        printf("sockfs: Socket not in valid state for receiving\n");
        return -1;
    }
    
    // Check if there's data in the read queue
    if (!sock->read_queue.head) {
        return 0; // No data available
    }
    
    // Dequeue the first message
    list_node_t* node = list_pop_head(&sock->read_queue);
    if (!node) return 0;
    
    sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
    
    // Copy data to user buffer
    size_t to_copy = msg->len < len ? msg->len : len;
    memcpy(buf, msg->data, to_copy);
    
    // Free the message
    kfree(msg->data);
    kfree(msg);
    
    return to_copy;
}

static int sockfs_socket_release(dentry_t* socket_node, file_t* file) {
    return sockfs_release(socket_node->d_inode, file);
}

/* ================
   MOUNT/UNMOUNT
   ================ */

dentry_t* sockfs_mount(file_system_type_t* fs_type, int flags,
                       block_device_t* dev_name, void* data) {
    // Create superblock
    super_block_t* sb = alloc_superblock(fs_type, &sockfs_sb_ops);
    if (!sb) return NULL;
    
    sb->s_magic = 0x534F434B; // 'SOCK'
    sb->block_size = 0; // No blocks - it's a pseudo filesystem
    sb->device = NULL; // No device
    
    // Create root inode
    inode_t* root_inode = alloc_inode(0, UMODE_IFDIR | 0755, 0,
                                      &sockfs_dir_inode_ops, NULL);
    if (!root_inode) {
        kfree(sb);
        return NULL;
    }
    
    root_inode->i_sb = sb;
    
    // Create root dentry
    sockfs_root = alloc_dentry("", root_inode, NULL);
    if (!sockfs_root) {
        kfree(root_inode);
        kfree(sb);
        return NULL;
    }
    
    sockfs_root->d_sb = sb;
    sb->s_root = sockfs_root;
    
    printf("sockfs: Mounted successfully\n");
    
    return sockfs_root;
}

void sockfs_kill_sb(super_block_t* sb) {
    if (!sb) return;
    
    // TODO: Clean up all socket inodes and dentries
    
    printf("sockfs: Unmounted\n");
}

/* ================
   PUBLIC API
   ================ */

/**
 * Create a socket in sockfs with the given name
 */
dentry_t* sockfs_create_socket(const char* name, sock_type_t type) {
    if (!sockfs_root || !name) return NULL;
    
    // Check if socket already exists
    if (sockfs_find_socket(name)) {
        printf("sockfs: Socket '%s' already exists\n", name);
        return NULL;
    }
    
    // Create socket inode
    inode_t* inode = sockfs_create_inode(sockfs_root->d_sb, type);
    if (!inode) return NULL;
    
    // Create dentry
    dentry_t* dentry = alloc_dentry(name, inode, sockfs_root);
    if (!dentry) {
        kfree(inode->private); // Free socket
        kfree(inode);
        return NULL;
    }
    
    dentry->d_sb = sockfs_root->d_sb;
    
    // Add to sockfs root's children
    list_push_tail(&sockfs_root->children, &dentry->node);
    
    return dentry;
}

/**
 * Lookup a socket by name in sockfs
 */
dentry_t* sockfs_lookup_socket(const char* name) {
    return sockfs_find_socket(name);
}

/**
 * Clean up socket queues
 */
static void sockfs_cleanup_queues(socket_t* sock) {
    if (!sock) return;
    
    // Clean up accept queue
    while (sock->accept_queue.head) {
        list_node_t* node = list_pop_head(&sock->accept_queue);
        sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
        kfree(msg);
    }
    
    // Clean up peer queue
    while (sock->peer_queue.head) {
        list_node_t* node = list_pop_head(&sock->peer_queue);
        sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
        kfree(msg);
    }
    
    // Clean up read queue
    while (sock->read_queue.head) {
        list_node_t* node = list_pop_head(&sock->read_queue);
        sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
        if (msg->data) kfree(msg->data);
        kfree(msg);
    }
    
    // Clean up write queue
    while (sock->write_queue.head) {
        list_node_t* node = list_pop_head(&sock->write_queue);
        sock_msg_t* msg = (sock_msg_t*)((uint8_t*)node - offsetof(sock_msg_t, node));
        if (msg->data) kfree(msg->data);
        kfree(msg);
    }
}

/**
 * Remove a socket from sockfs
 */
int sockfs_unlink_socket(const char* name) {
    dentry_t* dentry = sockfs_find_socket(name);
    if (!dentry) return -1;
    
    // Remove from parent's children list
    list_remove(&dentry->node);
    
    // Free socket data
    if (dentry->d_inode && dentry->d_inode->private) {
        socket_t* sock = (socket_t*)dentry->d_inode->private;
        sockfs_cleanup_queues(sock);
        kfree(sock);
    }
    
    // Free inode
    if (dentry->d_inode) {
        kfree(dentry->d_inode);
    }
    
    // Free dentry
    if (dentry->d_name) {
        kfree((void*)dentry->d_name);
    }
    kfree(dentry);
    
    return 0;
}

/**
 * Get socket operations for sockfs
 */
/**
 * Get socket operations for sockfs
 */
const socket_ops_t* sockfs_get_socket_ops(void) {
    return &sockfs_socket_ops;
}

/**
 * List all sockets in sockfs
 */
void sockfs_list_sockets(void) {
    if (!sockfs_root) {
        printf("Sockfs not initialized\n");
        return;
    }
    
    printf("Sockets in sockfs:\n");
    printf("NAME              TYPE      STATE\n");
    printf("=====================================\n");
    
    list_node_t* node = sockfs_root->children.head;
    if (!node) {
        printf("(no sockets)\n");
        return;
    }
    
    while (node) {
        dentry_t* child = (dentry_t*)((uint8_t*)node - offsetof(dentry_t, node));
        if (child && child->d_inode && child->d_inode->private) {
            socket_t* sock = (socket_t*)child->d_inode->private;
            
            const char* type_str = "UNKNOWN";
            switch (sock->type) {
                case SOCK_TYPE_STREAM: type_str = "STREAM"; break;
                case SOCK_TYPE_DGRAM:  type_str = "DGRAM "; break;
                case SOCK_TYPE_RAW:    type_str = "RAW   "; break;
            }
            
            const char* state_str = "UNKNOWN";
            switch (sock->state) {
                case SOCK_STATE_CLOSED:     state_str = "CLOSED    "; break;
                case SOCK_STATE_OPEN:       state_str = "OPEN      "; break;
                case SOCK_STATE_LISTENING:  state_str = "LISTENING "; break;
                case SOCK_STATE_CONNECTED:  state_str = "CONNECTED "; break;
            }
            
            printf("%-16s  %-8s  %s\n", 
                   child->d_name ? child->d_name : "(null)",
                   type_str, 
                   state_str);
        }
        node = node->next;
    }
}

/* ================
   INITIALIZATION
   ================ */

/**
 * Initialize sockfs - must be called during system initialization
 */
int sockfs_init(void) {
    // Mount sockfs (pseudo-filesystem, no device needed)
    dentry_t* root = sockfs_mount(&sockfs_fs_type, 0, NULL, NULL);
    return root ? 0 : -1;
}

/* ================
   FILESYSTEM TYPE
   ================ */

file_system_type_t sockfs_fs_type = {
    .name = "sockfs",
    .fs_flags = 0,
    .mount = sockfs_mount,
    .kill_sb = sockfs_kill_sb,
};