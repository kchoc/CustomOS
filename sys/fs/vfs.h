#ifndef FS_VFS_H
#define FS_VFS_H

#include "file.h"
#include "vnode.h"

#include <sys/device.h>

#include <kern/compiler.h>
#include <kern/socket.h>

#include <inttypes.h>
#include <list.h>

/* ===========
   DIR CONTEXT
   =========== */
typedef struct dir_context {
    unsigned int pos;
    bool (*actor)(dir_context_t* ctx, const char* name, int namelen, uint32_t ino,
                  uint32_t file_size, unsigned int type);
} dir_context_t;

/* ==========
   OPERATIONS
   ========== */

// typedef struct socket_ops {
//     int        (*listen)           (dentry_t* socket_node, int backlog);
//     file_t*    (*accept)           (dentry_t* socket_node, int flags);
//     int        (*connect)          (dentry_t* socket_node, file_t* file, int flags);
//     ssize_t    (*sendmsg)          (file_t* file, const void* buf, size_t len, int flags);
//     ssize_t    (*recvmsg)          (file_t* file,       void* buf, size_t len, int flags);
//     int        (*release)          (dentry_t* socket_node, file_t* file);
// } socket_ops_t;

/* ==========
    VFS MODES
    ========= */

/* File modes */

/* File is open for reading */
#define FMODE_READ 0x1
/* File is open for writing */
#define FMODE_WRITE 0x2
/* File supports seeking */
#define FMODE_LSEEK 0x4
/* File is opened for reading using pread */
#define FMODE_PREAD 0x8
/* File is opened for writing using pwrite */
#define FMODE_PWRITE 0x10
/* File is opened for execution */
#define FMODE_EXEC 0x20

/* Vnode modes (file types) */

/* File type mask */
#define UMODE_IFMT 0xF000
/* Regular file */
#define UMODE_IFREG 0x8000
/* Directory */
#define UMODE_IFDIR 0x4000
/* Character device */
#define UMODE_IFCHR 0x2000
/* Block device */
#define UMODE_IFBLK 0x6000
/* FIFO */
#define UMODE_IFIFO 0x1000
/* Symbolic link */
#define UMODE_IFLNK 0xA000
/* Socket */
#define UMODE_IFSOCK 0xC000

extern vnode_t* root_vnode;

/* Block Device Register */
int       vfs_register_device(device_t* bdev);
device_t* vfs_get_device(const char* device_name);
void      vfs_list_devices(void);

int vfs_init(void);
int vfs_mount_root(const char* device_name);

/* File operations */
file_t* vfs_open(const char* path, int flags, umode_t mode);
void    vfs_close(file_t* file);
ssize_t vfs_read(file_t* file, void* __user buf, size_t count, size_t offset);
ssize_t vfs_write(file_t* file, const void* __user buf, size_t count, size_t offset);
int     vfs_llseek(file_t* file, loff_t offset, int whence);

int vfs_getdirent(file_t* file, char* __user buf, size_t count, int offset);

/* Socket operations */
int     vfs_socket_create(const char* path, sock_type_t type, umode_t mode);
file_t* vfs_socket_connect(const char* path, int flags);
file_t* vfs_socket_accept(file_t* socket_file, int flags);
ssize_t vfs_socket_send(file_t* file, const void* __user buf, size_t len, int flags);
ssize_t vfs_socket_recv(file_t* file, void* __user buf, size_t len, int flags);
int     vfs_socket_unlink(const char* path);

void vfs_print_mounts(void);
void vfs_ls(const char* path);

#endif // FS_VFS_H
