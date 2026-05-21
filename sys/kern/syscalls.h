#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <libkern/common.h>

#include <inttypes.h>
#include <stddef.h>

#define SYSCALL_COUNT 577

#define SYSCALL_GENERIC     0
#define SYSCALL_EXIT        1
#define SYSCALL_FORK        2
#define SYSCALL_READ        3
#define SYSCALL_WRITE       4
#define SYSCALL_OPEN        5
#define SYSCALL_CLOSE       6
#define SYSCALL_WAIT4       7
#define SYSCALL_CREATE_OLD  8
#define SYSCALL_LINK        9
#define SYSCALL_UNLINK      10
#define SYSCALL_EXEC        11
#define SYSCALL_CHDIR       12
#define SYSCALL_FCHDIR      13
#define SYSCALL_MKNOD       14
#define SYSCALL_CHMOD       15
#define SYSCALL_CHOWN       16
#define SYSCALL_BRK         17
#define SYSCALL_GETFSSTAT   18
#define SYSCALL_LSEEK_OLD   19
#define SYSCALL_GETPID      20
#define SYSCALL_MOUNT       21
#define SYSCALL_UMOUNT      22
#define SYSCALL_SETUID      23
#define SYSCALL_GETUID      24
#define SYSCALL_GETEUID     25
#define SYSCALL_PTRACE      26
#define SYSCALL_RECVMSG     27
#define SYSCALL_SENDMSG     28
#define SYSCALL_RECVFROM    29
#define SYSCALL_ACCEPT      30
#define SYSCALL_GETPEERNAME 31
#define SYSCALL_GETSOCKNAME 32
#define SYSCALL_ACCESS      33
#define SYSCALL_CHFLAGS     34
#define SYSCALL_FCHFLAGS    35
#define SYSCALL_SYNC        36
#define SYSCALL_KILL        37
#define SYSCALL_STAT_OLD    38
#define SYSCALL_GETPPID     39
#define SYSCALL_LSTAT_OLD   40
#define SYSCALL_DUP         41
#define SYSCALL_PIPE        42
#define SYSCALL_GETEGID     43
#define SYSCALL_PROFILING   44
#define SYSCALL_KTRACE      45
#define SYSCALL_SIGACTION   46
#define SYSCALL_GETGID      47
#define SYSCALL_SIGPROCMASK 48
#define SYSCALL_GETLOGIN    49
#define SYSCALL_SETLOGIN    50
#define SYSCALL_ACCT        51
#define SYSCALL_SIGPENDING  52
#define SYSCALL_SIGALTSTACK 53
#define SYSCALL_IOCTL       54
#define SYSCALL_REBOOT      55
#define SYSCALL_REVOKE      56
#define SYSCALL_SYMLINK     57
#define SYSCALL_READLINK    58
#define SYSCALL_EXECVE      59
#define SYSCALL_UMASK       60
#define SYSCALL_CHROOT      61

#define SYSCALL_FSYNC 95

#define SYSCALL_FCHOWN 123
#define SYSCALL_FCHMOD 124

#define SYSCALL_RENAME 128

#define SYSCALL_MKDIR  136
#define SYSCALL_RMDIR  137
#define SYSCALL_UTIMES 138

#define SYSCALL_QUOTACTL 148

#define SYSCALL_LGETFH 160
#define SYSCALL_GETFH  161

#define SYSCALL_PATHCONF  191
#define SYSCALL_FPATHCONF 192

#define SYSCALL_GETDIRENT 554

#define SYSCALL_PRINT 100

#define SYSCALL_THREAD_NEW 455

// Macro to define syscall function prototypes so that syscalls using < 5 args can be defined easily
#define SYSCALL1 uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL2 uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL3 uint32_t arg4, uint32_t arg5
#define SYSCALL4 uint32_t arg5

extern void* g_syscalls[];

typedef int (*syscall_fn_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void syscalls_init();
int  syscall(uint32_t syscall_id, int arg_count, ...);

/* Process syscalls */
int syscall_exit(registers_t* regs);

/* I/O syscalls */
int syscall_print(const char* str, SYSCALL1);
int syscall_open(const char* path, int flags, uint32_t mode, SYSCALL2);
int syscall_close(int fd, SYSCALL1);
int syscall_read(int fd, void* buf, size_t count, SYSCALL2);
int syscall_write(int fd, const void* buf, size_t count, SYSCALL2);
int syscall_getdirent(int fd, char* buf, size_t count, int offset, SYSCALL2);

/* Socket syscalls */
int syscall_socket(int type, SYSCALL1);
int syscall_connect(int sockfd, const char* path, SYSCALL2);
int syscall_listen(int sockfd, int backlog, SYSCALL2);
int syscall_accept(int sockfd, SYSCALL1);
int syscall_send(int sockfd, const void* buf, size_t len, int flags, SYSCALL1);
int syscall_recv(int sockfd, void* buf, size_t len, int flags, SYSCALL1);
int syscall_unlink(const char* path, SYSCALL1);

/* Memory syscalls */
void* syscall_mmap(uintptr_t addr, size_t length, int prot, int flags, SYSCALL1);

/* Process syscalls */
int syscall_fork(SYSCALL1);

/* Exec syscall */
int syscall_execve(const char* path, char* const argv[], char* const envp[], SYSCALL2);

#endif // SYSCALLS_H
