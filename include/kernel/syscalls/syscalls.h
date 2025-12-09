#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "kernel/types.h"
#include "types/common.h"

#define SYSCALL_COUNT 256

#define SYSCALL_EXIT    0
#define SYSCALL_PRINT   1
#define SYSCALL_OPEN    2
#define SYSCALL_CLOSE   3
#define SYSCALL_READ    4
#define SYSCALL_WRITE   5
#define SYSCALL_SOCKET  6
#define SYSCALL_CONNECT 7
#define SYSCALL_LISTEN  8
#define SYSCALL_ACCEPT  9
#define SYSCALL_SEND    10
#define SYSCALL_RECV    11
#define SYSCALL_UNLINK      12
#define SYSCALL_MMAP        13
#define SYSCALL_WIN_CREATE  14
#define SYSCALL_WIN_DESTROY 15
#define SYSCALL_WIN_UPDATE  16
#define SYSCALL_WIN_GETBUF  17
#define SYSCALL_READ_STDIN  18

// Macro to define syscall function prototypes so that syscalls using < 5 args can be defined easily
#define SYSCALL1 uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL2 uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL3 uint32_t arg4, uint32_t arg5
#define SYSCALL4 uint32_t arg5

extern void* g_syscalls[];

typedef int (*syscall_fn_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void syscalls_init();
int syscall(uint32_t syscall_id, int arg_count, ...);

/* Process syscalls */
int syscall_exit(registers_t* regs);

/* I/O syscalls */
int syscall_print(const char* str, SYSCALL1);
int syscall_open(const char* path, int flags, uint32_t mode, SYSCALL2);
int syscall_close(int fd, SYSCALL1);
int syscall_read(int fd, void* buf, size_t count, SYSCALL2);
int syscall_write(int fd, const void* buf, size_t count, SYSCALL2);

/* Socket syscalls */
int syscall_socket(int type, SYSCALL1);
int syscall_connect(int sockfd, const char* path, SYSCALL2);
int syscall_listen(int sockfd, int backlog, SYSCALL2);
int syscall_accept(int sockfd, SYSCALL1);
int syscall_send(int sockfd, const void* buf, size_t len, int flags, SYSCALL1);
int syscall_recv(int sockfd, void* buf, size_t len, int flags, SYSCALL1);
int syscall_unlink(const char* path, SYSCALL1);

/* Memory syscalls */
void* syscall_mmap(void* addr, size_t length, int prot, int flags, SYSCALL1);

/* Window syscalls */
int syscall_win_create(const char* title, int x, int y, int width, int height);
int syscall_win_destroy(uint32_t wid, SYSCALL1);
int syscall_win_update(uint32_t wid, SYSCALL1);
void* syscall_win_getbuf(uint32_t wid, SYSCALL1);

/* Input syscalls */
int syscall_read_stdin(char* buffer, int count, SYSCALL1);

#endif //SYSCALLS_H