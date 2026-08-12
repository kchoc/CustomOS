#include "syscalls.h"
#include "exec.h"
#include "fd.h"
#include "process.h"
#include "terminal.h"

#include <dev/input/keyboard.h>

#include <fs/file.h>
#include <fs/vfs.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <sys/pcpu.h>

#include <libkern/common.h>

#include <stdarg.h>
#include <string.h>

void* g_syscalls[SYSCALL_COUNT];

void syscalls_init()
{
    memset((uint8_t*)g_syscalls, 0, sizeof(void*) * SYSCALL_COUNT);

    g_syscalls[SYSCALL_PRINT] = syscall_print;

    // I/O syscalls
    g_syscalls[SYSCALL_OPEN]      = syscall_open;
    g_syscalls[SYSCALL_CLOSE]     = syscall_close;
    g_syscalls[SYSCALL_READ]      = syscall_read;
    g_syscalls[SYSCALL_WRITE]     = syscall_write;
    g_syscalls[SYSCALL_GETDIRENT] = syscall_getdirent;

    // Process Syscalls
    g_syscalls[SYSCALL_FORK]   = syscall_fork;
    g_syscalls[SYSCALL_EXECVE] = syscall_execve;
    g_syscalls[SYSCALL_EXIT]   = syscall_exit;
}

int syscall_exit(registers_t* regs)
{
    thread_exit(regs);
    return 0; // Success
}

int syscall_print(const char* str, SYSCALL1)
{
    if (!str)
        return -1; // Invalid string
    printf("%s", str);
    return 0; // Success
}

/* ================
   FILE I/O SYSCALLS
   ================ */

int syscall_open(const char* path, int flags, uint32_t mode, SYSCALL2)
{
    if (!path)
        return -1;

    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc)
        return -1;

    printf("syscall_open: Opening file '%s' with flags 0x%x for process %s (PID %d)\n", path, flags,
           proc->name, proc->pid);

    // Open the file via VFS
    file_t* file = vfs_open(path, flags, mode);
    if (!file)
        return -1;

    // Allocate a file descriptor
    int fd = fd_alloc(proc, file, 0);
    if (fd < 0) {
        vfs_close(file);
        return -1;
    }

    return fd;
}

int syscall_close(int fd, SYSCALL1)
{
    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc)
        return -1;

    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file)
        return -1;

    // Close via VFS
    vfs_close(file);

    // Close the file descriptor
    return fd_close(proc, fd);
}

int syscall_read(int fd, void* buf, size_t count, SYSCALL2)
{
    if (!buf)
        return -1;

    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc)
        return -1;

    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file)
        return -1;

    // Read via VFS
    return vfs_read(file, buf, count, 0);
}

int syscall_write(int fd, const void* buf, size_t count, SYSCALL2)
{
    if (!buf)
        return -1;

    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc)
        return -1;

    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file)
        return -1;

    // Write via VFS
    return vfs_write(file, buf, count, 0);
}

int syscall_getdirent(int fd, char* buf, size_t count, int offset, SYSCALL2)
{
    if (!buf)
        return -1;

    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc)
        return -1;

    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file)
        return -1;

    // Get directory entry via VFS
    return vfs_getdirent(file, buf, count, offset);
}

/* ================
   SOCKET SYSCALLS
   ================ */

int syscall_socket(int type, SYSCALL1)
{
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc || !proc->fd_table) return -1;
    //
    // // Generate a unique socket name for this process
    // static uint32_t socket_counter = 0;
    // char sock_name[32];
    // snprintf(sock_name, sizeof(sock_name), "sock_%u_%u", proc->pid, socket_counter++);
    //
    // // Create socket in sockfs
    // dentry_t* sock_dentry = sockfs_create_socket(sock_name, type);
    // if (!sock_dentry) return -1;
    //
    // // Open as a file
    // file_t* file = alloc_file(sock_dentry, FMODE_READ | FMODE_WRITE);
    // if (!file) return -1;
    //
    // // Open the socket
    // if (file->f_ops && file->f_ops->open) {
    //     if (file->f_ops->open(file->f_inode, file)) {
    //         kfree(file);
    //         return -1;
    //     }
    // }
    //
    // // Allocate a file descriptor
    // int fd = fd_alloc(proc, file, 0);
    // if (fd < 0) {
    //     kfree(file);
    //     return -1;
    // }
    //
    // return fd;
    return 0;
}

int syscall_connect(int sockfd, const char* path, SYSCALL2)
{
    // if (!path) return -1;
    //
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc) return -1;
    //
    // // Get the socket file from fd
    // file_t* sock_file = fd_get_file(proc, sockfd);
    // if (!sock_file) return -1;
    //
    // // Lookup the target socket
    // dentry_t* target_dentry = sockfs_lookup_socket(path);
    // if (!target_dentry) return -1;
    //
    // // Connect
    // const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    // if (!sock_ops || !sock_ops->connect) return -1;
    //
    // return sock_ops->connect(target_dentry, sock_file, 0);
    return 0;
}

int syscall_listen(int sockfd, int backlog, SYSCALL2)
{
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc) return -1;
    //
    // // Get the socket file from fd
    // file_t* sock_file = fd_get_file(proc, sockfd);
    // if (!sock_file) return -1;
    //
    // // Listen
    // const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    // if (!sock_ops || !sock_ops->listen) return -1;
    //
    // return sock_ops->listen(sock_file->f_dentry, backlog);
    return 0;
}

int syscall_accept(int sockfd, SYSCALL1)
{
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc) return -1;
    //
    // // Get the socket file from fd
    // file_t* listen_file = fd_get_file(proc, sockfd);
    // if (!listen_file) return -1;
    //
    // // Accept connection
    // const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    // if (!sock_ops || !sock_ops->accept) return -1;
    //
    // file_t* client_file = sock_ops->accept(listen_file->f_dentry, 0);
    // if (!client_file) return -1;
    //
    // // Allocate fd for the accepted connection
    // int client_fd = fd_alloc(proc, client_file, 0);
    // if (client_fd < 0) {
    //     vfs_close(client_file);
    //     return -1;
    // }
    //
    // return client_fd;
    return 0;
}

int syscall_send(int sockfd, const void* buf, size_t len, int flags, SYSCALL1)
{
    // if (!buf) return -1;
    //
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc) return -1;
    //
    // // Get the socket file from fd
    // file_t* sock_file = fd_get_file(proc, sockfd);
    // if (!sock_file) return -1;
    //
    // // Send via socket ops
    // return vfs_socket_send(sock_file, buf, len, flags);
    return 0;
}

int syscall_recv(int sockfd, void* buf, size_t len, int flags, SYSCALL1)
{
    // if (!buf) return -1;
    //
    // proc_t* proc = PCPU_GET(current_thread)->proc;
    // if (!proc) return -1;
    //
    // // Get the socket file from fd
    // file_t* sock_file = fd_get_file(proc, sockfd);
    // if (!sock_file) return -1;
    //
    // // Receive via socket ops
    // return vfs_socket_recv(sock_file, buf, len, flags);
    return 0;
}

int syscall_unlink(const char* path, SYSCALL1)
{
    // if (!path) return -1;
    //
    // // Unlink socket
    // return vfs_socket_unlink(path);
    return 0;
}

/* ================
   SYSCALL WRAPPER
   ================ */

int syscall(uint32_t syscall_id, int arg_count, ...)
{
    uint32_t args[5] = {0};

    va_list ap;
    va_start(ap, arg_count);
    for (int i = 0; i < arg_count && i < 5; ++i) {
        args[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);

    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)        // output: return value in eax
                 : "a"(syscall_id), // input: syscall number in eax
                   "b"(args[0]),    // arg1 in ebx
                   "c"(args[1]),    // arg2 in ecx
                   "d"(args[2]),    // arg3 in edx
                   "S"(args[3]),    // arg4 in esi
                   "D"(args[4])     // arg5 in edi
                 : "memory");

    return ret;
}

void* syscall_mmap(uintptr_t addr, size_t length, int prot, int flags, SYSCALL1)
{
    proc_t* proc = get_proc_from_thread(PCPU_GET(current_thread));
    if (!proc || !proc->vmspace)
        return NULL;

    printf("syscall_mmap: Mapping %u bytes at %p\n", length, addr);

    // Map memory in process's VM space
    if (vm_map_anon(proc->vmspace, &addr, length, prot, flags, VM_MAP_F_NONE) < 0) {
        return NULL;
    }

    return (void*)addr;
}

int syscall_fork(SYSCALL1)
{
    proc_t* parent = get_proc_from_thread(PCPU_GET(current_thread));
    proc_t* child;
    printf("syscall_fork: Forking process %s (PID %d)\n", parent->name, parent->pid);
    int res = fork_process(PCPU_GET(current_thread), 0, &child);
    printf("syscall_fork: Forked process %s (PID %d)\n", child->name, child->pid);
    if (res)
        return res;
    return child->pid; // Return child's PID to parent, 0 to child
}

int syscall_execve(const char* path, char* const argv[], char* const envp[], SYSCALL2)
{
    if (!path)
        return -1;

    printf("syscall_execve: Executing %s (PID=%d)\n", path,
           get_proc_from_thread(PCPU_GET(current_thread))->pid);

    return execve(path, argv, envp);
}
