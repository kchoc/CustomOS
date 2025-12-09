#ifndef FD_H
#define FD_H

#include "kernel/types.h"

typedef struct file file_t;
typedef struct process proc_t;

#define MAX_FDS 256

/* Standard file descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* File descriptor entry */
typedef struct fd_entry {
    file_t* file;       // Pointer to the open file
    int flags;          // Flags (close-on-exec, etc.)
    int ref_count;      // Reference count for dup()
} fd_entry_t;

/* File descriptor table */
typedef struct fd_table {
    fd_entry_t* fds[MAX_FDS];  // Array of file descriptor entries
    int next_fd;                // Next available fd (optimization)
} fd_table_t;

/* File descriptor table management */
fd_table_t* fd_table_create(void);
void fd_table_destroy(fd_table_t* table);
fd_table_t* fd_table_clone(fd_table_t* src);

/* File descriptor operations */
int fd_alloc(proc_t* proc, file_t* file, int flags);
file_t* fd_get_file(proc_t* proc, int fd);
int fd_close(proc_t* proc, int fd);
int fd_dup(proc_t* proc, int oldfd);
int fd_dup2(proc_t* proc, int oldfd, int newfd);

/* Initialize standard file descriptors for a process */
int fd_init_stdio(proc_t* proc);

#endif // FD_H
