#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#include <stddef.h>
#include <stdint.h>

// Syscall numbers (must match kernel definitions)
#define SYSCALL_EXIT  1
#define SYSCALL_FORK  2
#define SYSCALL_READ  3
#define SYSCALL_WRITE 4
#define SYSCALL_OPEN  5
#define SYSCALL_CLOSE 6

#define SYSCALL_GETDIRENT 554

#define SYSCALL_PRINT  100
#define SYSCALL_EXECVE 59

// Memory mapping flags
#define MMAP_FRAMEBUFFER 0x1

// Socket types
#define SOCK_TYPE_STREAM 1
#define SOCK_TYPE_DGRAM  2
#define SOCK_TYPE_RAW    3

// File flags
#define O_RDONLY 0x0001
#define O_WRONLY 0x0002
#define O_RDWR   0x0003
#define O_CREAT  0x0100
#define O_TRUNC  0x0200
#define O_APPEND 0x0400

// Standard file descriptors
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/**
 * Low-level syscall wrapper - invokes int 0x80
 * Users should use the higher-level wrappers below instead
 */
static inline int syscall(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, uint32_t arg3,
                          uint32_t arg4, uint32_t arg5)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(syscall_id), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
                     : "memory");
    return ret;
}

// Process syscalls
static inline void exit(int status)
{
    syscall(SYSCALL_EXIT, status, 0, 0, 0, 0);
}

// I/O syscalls
static inline int print(const char* str)
{
    return syscall(SYSCALL_PRINT, (uint32_t)str, 0, 0, 0, 0);
}

static inline int open(const char* path, int flags, uint32_t mode)
{
    return syscall(SYSCALL_OPEN, (uint32_t)path, flags, mode, 0, 0);
}

static inline int close(int fd)
{
    return syscall(SYSCALL_CLOSE, fd, 0, 0, 0, 0);
}

static inline int read(int fd, void* buf, size_t count)
{
    return syscall(SYSCALL_READ, fd, (uint32_t)buf, count, 0, 0);
}

static inline int write(int fd, const void* buf, size_t count)
{
    return syscall(SYSCALL_WRITE, fd, (uint32_t)buf, count, 0, 0);
}

static inline int getdirent(int fd, char* buf, size_t count, uintptr_t offset)
{
    return syscall(SYSCALL_GETDIRENT, fd, (uint32_t)buf, count, 0, 0);
}

static inline int fork()
{
    return syscall(SYSCALL_FORK, 0, 0, 0, 0, 0);
}

static inline int execve(const char* path, char* const argv[], char* const envp[])
{
    return syscall(SYSCALL_EXECVE, (uint32_t)path, (uint32_t)argv, (uint32_t)envp, 0, 0);
}

#endif // USER_SYSCALLS_H
