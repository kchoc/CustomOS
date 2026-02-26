#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

// Syscall numbers (must match kernel definitions)
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

// Memory mapping flags
#define MMAP_FRAMEBUFFER    0x1

// Socket types
#define SOCK_TYPE_STREAM    1
#define SOCK_TYPE_DGRAM     2
#define SOCK_TYPE_RAW       3

// File flags
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0003
#define O_CREAT     0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

// Standard file descriptors
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/**
 * Low-level syscall wrapper - invokes int 0x80
 * Users should use the higher-level wrappers below instead
 */
static inline int syscall(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, 
                          uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(syscall_id), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
        : "memory"
    );
    return ret;
}

// Process syscalls
static inline void exit(int status) {
    syscall(SYSCALL_EXIT, status, 0, 0, 0, 0);
}

// I/O syscalls
static inline int print(const char* str) {
    return syscall(SYSCALL_PRINT, (uint32_t)str, 0, 0, 0, 0);
}

static inline int open(const char* path, int flags, uint32_t mode) {
    return syscall(SYSCALL_OPEN, (uint32_t)path, flags, mode, 0, 0);
}

static inline int close(int fd) {
    return syscall(SYSCALL_CLOSE, fd, 0, 0, 0, 0);
}

static inline int read(int fd, void* buf, size_t count) {
    return syscall(SYSCALL_READ, fd, (uint32_t)buf, count, 0, 0);
}

static inline int write(int fd, const void* buf, size_t count) {
    return syscall(SYSCALL_WRITE, fd, (uint32_t)buf, count, 0, 0);
}

// Socket syscalls
static inline int socket(int type) {
    return syscall(SYSCALL_SOCKET, type, 0, 0, 0, 0);
}

static inline int connect(int sockfd, const char* path) {
    return syscall(SYSCALL_CONNECT, sockfd, (uint32_t)path, 0, 0, 0);
}

static inline int listen(int sockfd, int backlog) {
    return syscall(SYSCALL_LISTEN, sockfd, backlog, 0, 0, 0);
}

static inline int accept(int sockfd) {
    return syscall(SYSCALL_ACCEPT, sockfd, 0, 0, 0, 0);
}

static inline int send(int sockfd, const void* buf, size_t len, int flags) {
    return syscall(SYSCALL_SEND, sockfd, (uint32_t)buf, len, flags, 0);
}

static inline int recv(int sockfd, void* buf, size_t len, int flags) {
    return syscall(SYSCALL_RECV, sockfd, (uint32_t)buf, len, flags, 0);
}

static inline int unlink(const char* path) {
    return syscall(SYSCALL_UNLINK, (uint32_t)path, 0, 0, 0, 0);
}

static inline void* mmap(void* addr, size_t length, int prot, int flags) {
    return (void*)syscall(SYSCALL_MMAP, (uint32_t)addr, length, prot, flags, 0);
}

// Window management syscalls
static inline int win_create(const char* title, int x, int y, int width, int height) {
    return syscall(SYSCALL_WIN_CREATE, (uint32_t)title, x, y, width, height);
}

static inline int win_destroy(int wid) {
    return syscall(SYSCALL_WIN_DESTROY, wid, 0, 0, 0, 0);
}

static inline int win_update(int wid) {
    return syscall(SYSCALL_WIN_UPDATE, wid, 0, 0, 0, 0);
}

static inline void* win_getbuf(int wid) {
    return (void*)syscall(SYSCALL_WIN_GETBUF, wid, 0, 0, 0, 0);
}

static inline int read_stdin(char* buffer, int count) {
    return (int)syscall(SYSCALL_READ_STDIN, (uint32_t)buffer, count, 0, 0, 0);
}

#endif // USER_SYSCALLS_H
