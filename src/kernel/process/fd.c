#include "kernel/process/fd.h"
#include "kernel/process/process.h"
#include "kernel/filesystem/file.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"
#include "types/string.h"

/* ================
   FD TABLE MANAGEMENT
   ================ */

/**
 * Create a new file descriptor table
 */
fd_table_t* fd_table_create(void) {
    fd_table_t* table = kmalloc(sizeof(fd_table_t));
    if (!table) return NULL;
    
    memset(table, 0, sizeof(fd_table_t));
    table->next_fd = 3; // Start after stdin, stdout, stderr
    
    return table;
}

/**
 * Destroy a file descriptor table
 */
void fd_table_destroy(fd_table_t* table) {
    if (!table) return;
    
    // Close all open file descriptors
    for (int i = 0; i < MAX_FDS; i++) {
        if (table->fds[i]) {
            fd_entry_t* entry = table->fds[i];
            
            entry->ref_count--;
            if (entry->ref_count <= 0) {
                // No more references, close the file
                if (entry->file) {
                    // Note: vfs_close should be called, but we need to avoid circular dependency
                    // For now, just decrement ref count
                    entry->file->ref_count--;
                }
                kfree(entry);
            }
            table->fds[i] = NULL;
        }
    }
    
    kfree(table);
}

/**
 * Clone a file descriptor table (for fork)
 */
fd_table_t* fd_table_clone(fd_table_t* src) {
    if (!src) return NULL;
    
    fd_table_t* table = fd_table_create();
    if (!table) return NULL;
    
    // Copy all file descriptors
    for (int i = 0; i < MAX_FDS; i++) {
        if (src->fds[i]) {
            table->fds[i] = src->fds[i];
            table->fds[i]->ref_count++; // Increment reference count
            
            if (table->fds[i]->file) {
                table->fds[i]->file->ref_count++; // Increment file reference
            }
        }
    }
    
    table->next_fd = src->next_fd;
    
    return table;
}

/* ================
   FD OPERATIONS
   ================ */

/**
 * Allocate a new file descriptor for the given file
 */
int fd_alloc(proc_t* proc, file_t* file, int flags) {
    if (!proc || !proc->fd_table || !file) return -1;
    
    fd_table_t* table = proc->fd_table;
    
    // Find the lowest available file descriptor
    int fd = -1;
    for (int i = table->next_fd; i < MAX_FDS; i++) {
        if (!table->fds[i]) {
            fd = i;
            break;
        }
    }
    
    // If not found after next_fd, search from beginning
    if (fd == -1) {
        for (int i = 0; i < table->next_fd && i < MAX_FDS; i++) {
            if (!table->fds[i]) {
                fd = i;
                break;
            }
        }
    }
    
    if (fd == -1) return -1; // No available file descriptors
    
    // Create fd entry
    fd_entry_t* entry = kmalloc(sizeof(fd_entry_t));
    if (!entry) return -1;
    
    entry->file = file;
    entry->flags = flags;
    entry->ref_count = 1;
    
    table->fds[fd] = entry;
    table->next_fd = fd + 1;
    
    // Increment file reference count
    file->ref_count++;
    
    return fd;
}

/**
 * Get the file associated with a file descriptor
 */
file_t* fd_get_file(proc_t* proc, int fd) {
    if (!proc || !proc->fd_table) return NULL;
    if (fd < 0 || fd >= MAX_FDS) return NULL;
    
    fd_entry_t* entry = proc->fd_table->fds[fd];
    if (!entry) return NULL;
    
    return entry->file;
}

/**
 * Close a file descriptor
 */
int fd_close(proc_t* proc, int fd) {
    if (!proc || !proc->fd_table) return -1;
    if (fd < 0 || fd >= MAX_FDS) return -1;
    
    fd_entry_t* entry = proc->fd_table->fds[fd];
    if (!entry) return -1; // Already closed
    
    entry->ref_count--;
    
    if (entry->ref_count <= 0) {
        // No more references to this fd entry
        if (entry->file) {
            entry->file->ref_count--;
            
            // If file has no more references, close it
            // Note: Should call vfs_close here, but avoiding circular dependency
            if (entry->file->ref_count <= 0) {
                // File operations release should be called
                // This will be handled by vfs_close in syscalls
            }
        }
        
        kfree(entry);
    }
    
    proc->fd_table->fds[fd] = NULL;
    
    // Update next_fd hint
    if (fd < proc->fd_table->next_fd) {
        proc->fd_table->next_fd = fd;
    }
    
    return 0;
}

/**
 * Duplicate a file descriptor
 */
int fd_dup(proc_t* proc, int oldfd) {
    if (!proc || !proc->fd_table) return -1;
    if (oldfd < 0 || oldfd >= MAX_FDS) return -1;
    
    fd_entry_t* old_entry = proc->fd_table->fds[oldfd];
    if (!old_entry) return -1; // Invalid fd
    
    // Allocate new fd for the same file
    int newfd = fd_alloc(proc, old_entry->file, old_entry->flags);
    if (newfd < 0) return -1;
    
    // Note: fd_alloc already incremented ref counts
    
    return newfd;
}

/**
 * Duplicate a file descriptor to a specific fd number
 */
int fd_dup2(proc_t* proc, int oldfd, int newfd) {
    if (!proc || !proc->fd_table) return -1;
    if (oldfd < 0 || oldfd >= MAX_FDS) return -1;
    if (newfd < 0 || newfd >= MAX_FDS) return -1;
    
    if (oldfd == newfd) return newfd; // Same fd, nothing to do
    
    fd_entry_t* old_entry = proc->fd_table->fds[oldfd];
    if (!old_entry) return -1; // Invalid source fd
    
    // Close newfd if it's already open
    if (proc->fd_table->fds[newfd]) {
        fd_close(proc, newfd);
    }
    
    // Create new fd entry at specific location
    fd_entry_t* entry = kmalloc(sizeof(fd_entry_t));
    if (!entry) return -1;
    
    entry->file = old_entry->file;
    entry->flags = old_entry->flags;
    entry->ref_count = 1;
    
    proc->fd_table->fds[newfd] = entry;
    
    // Increment file reference count
    old_entry->file->ref_count++;
    
    return newfd;
}

/**
 * Initialize standard file descriptors (stdin, stdout, stderr)
 */
int fd_init_stdio(proc_t* proc) {
    if (!proc || !proc->fd_table) return -1;
    
    // For now, stdin/stdout/stderr are NULL (no terminal file yet)
    // This should be set up to point to a terminal device or console
    // We'll just create placeholder entries
    
    for (int i = 0; i < 3; i++) {
        fd_entry_t* entry = kmalloc(sizeof(fd_entry_t));
        if (!entry) return -1;
        
        entry->file = NULL; // TODO: Set to actual console/terminal device
        entry->flags = 0;
        entry->ref_count = 1;
        
        proc->fd_table->fds[i] = entry;
    }
    
    return 0;
}
