#include "syscalls.h"
#include "process.h"
#include "fd.h"
#include "terminal.h"

#include <fs/vfs.h>
#include <fs/sockfs.h>
#include <fs/file.h>
#include <vm/kmalloc.h>
#include <vm/vm_map.h>
#include <wm/window.h>
#include <dev/input/keyboard.h>

#include <string.h>
#include <libkern/common.h>
#include <stdarg.h>

void* g_syscalls[SYSCALL_COUNT];

void syscalls_init() {
    memset((uint8_t*)g_syscalls, 0, sizeof(void*) * SYSCALL_COUNT);

    // Process syscalls
    g_syscalls[SYSCALL_EXIT] = syscall_exit;
    
    // I/O syscalls
    g_syscalls[SYSCALL_PRINT] = syscall_print;
    g_syscalls[SYSCALL_OPEN] = syscall_open;
    g_syscalls[SYSCALL_CLOSE] = syscall_close;
    g_syscalls[SYSCALL_READ] = syscall_read;
    g_syscalls[SYSCALL_WRITE] = syscall_write;
    
    // Socket syscalls
    g_syscalls[SYSCALL_SOCKET] = syscall_socket;
    g_syscalls[SYSCALL_CONNECT] = syscall_connect;
    g_syscalls[SYSCALL_LISTEN] = syscall_listen;
    g_syscalls[SYSCALL_ACCEPT] = syscall_accept;
    g_syscalls[SYSCALL_SEND] = syscall_send;
    g_syscalls[SYSCALL_RECV] = syscall_recv;
    g_syscalls[SYSCALL_UNLINK] = syscall_unlink;
    
    // Memory syscalls
    g_syscalls[SYSCALL_MMAP] = syscall_mmap;
    
    // Window syscalls
    g_syscalls[SYSCALL_WIN_CREATE] = syscall_win_create;
    g_syscalls[SYSCALL_WIN_DESTROY] = syscall_win_destroy;
    g_syscalls[SYSCALL_WIN_UPDATE] = syscall_win_update;
    g_syscalls[SYSCALL_WIN_GETBUF] = syscall_win_getbuf;
    
    // Input syscalls
    g_syscalls[SYSCALL_READ_STDIN] = syscall_read_stdin;
}

int syscall_exit(registers_t* regs) {
    printf("Process exiting...\n");
    thread_exit(regs);
    return 0; // Success
}

int syscall_print(const char* str, SYSCALL1) {
    if (!str) return -1; // Invalid string
    printf("%s", str);
    return 0; // Success
}

/* ================
   FILE I/O SYSCALLS
   ================ */

int syscall_open(const char* path, int flags, uint32_t mode, SYSCALL2) {
    if (!path) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Open the file via VFS
    file_t* file = vfs_open(path, flags, mode);
    if (!file) return -1;
    
    // Allocate a file descriptor
    int fd = fd_alloc(proc, file, 0);
    if (fd < 0) {
        vfs_close(file);
        return -1;
    }
    
    return fd;
}

int syscall_close(int fd, SYSCALL1) {
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file) return -1;
    
    // Close via VFS
    vfs_close(file);
    
    // Close the file descriptor
    return fd_close(proc, fd);
}

int syscall_read(int fd, void* buf, size_t count, SYSCALL2) {
    if (!buf) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file) return -1;
    
    // Read via VFS
    return vfs_read(file, buf, count, NULL);
}

int syscall_write(int fd, const void* buf, size_t count, SYSCALL2) {
    if (!buf) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the file from fd
    file_t* file = fd_get_file(proc, fd);
    if (!file) return -1;
    
    // Write via VFS
    return vfs_write(file, buf, count, NULL);
}

/* ================
   SOCKET SYSCALLS
   ================ */

int syscall_socket(int type, SYSCALL1) {
    proc_t* proc = get_current_process();
    if (!proc || !proc->fd_table) return -1;
    
    // Generate a unique socket name for this process
    static uint32_t socket_counter = 0;
    char sock_name[32];
    snprintf(sock_name, sizeof(sock_name), "sock_%u_%u", proc->pid, socket_counter++);
    
    // Create socket in sockfs
    dentry_t* sock_dentry = sockfs_create_socket(sock_name, type);
    if (!sock_dentry) return -1;
    
    // Open as a file
    file_t* file = alloc_file(sock_dentry, FMODE_READ | FMODE_WRITE);
    if (!file) return -1;
    
    // Open the socket
    if (file->f_ops && file->f_ops->open) {
        if (file->f_ops->open(file->f_inode, file)) {
            kfree(file);
            return -1;
        }
    }
    
    // Allocate a file descriptor
    int fd = fd_alloc(proc, file, 0);
    if (fd < 0) {
        kfree(file);
        return -1;
    }
    
    return fd;
}

int syscall_connect(int sockfd, const char* path, SYSCALL2) {
    if (!path) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the socket file from fd
    file_t* sock_file = fd_get_file(proc, sockfd);
    if (!sock_file) return -1;
    
    // Lookup the target socket
    dentry_t* target_dentry = sockfs_lookup_socket(path);
    if (!target_dentry) return -1;
    
    // Connect
    const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    if (!sock_ops || !sock_ops->connect) return -1;
    
    return sock_ops->connect(target_dentry, sock_file, 0);
}

int syscall_listen(int sockfd, int backlog, SYSCALL2) {
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the socket file from fd
    file_t* sock_file = fd_get_file(proc, sockfd);
    if (!sock_file) return -1;
    
    // Listen
    const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    if (!sock_ops || !sock_ops->listen) return -1;
    
    return sock_ops->listen(sock_file->f_dentry, backlog);
}

int syscall_accept(int sockfd, SYSCALL1) {
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the socket file from fd
    file_t* listen_file = fd_get_file(proc, sockfd);
    if (!listen_file) return -1;
    
    // Accept connection
    const socket_ops_t* sock_ops = sockfs_get_socket_ops();
    if (!sock_ops || !sock_ops->accept) return -1;
    
    file_t* client_file = sock_ops->accept(listen_file->f_dentry, 0);
    if (!client_file) return -1;
    
    // Allocate fd for the accepted connection
    int client_fd = fd_alloc(proc, client_file, 0);
    if (client_fd < 0) {
        vfs_close(client_file);
        return -1;
    }
    
    return client_fd;
}

int syscall_send(int sockfd, const void* buf, size_t len, int flags, SYSCALL1) {
    if (!buf) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the socket file from fd
    file_t* sock_file = fd_get_file(proc, sockfd);
    if (!sock_file) return -1;
    
    // Send via socket ops
    return vfs_socket_send(sock_file, buf, len, flags);
}

int syscall_recv(int sockfd, void* buf, size_t len, int flags, SYSCALL1) {
    if (!buf) return -1;
    
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    // Get the socket file from fd
    file_t* sock_file = fd_get_file(proc, sockfd);
    if (!sock_file) return -1;
    
    // Receive via socket ops
    return vfs_socket_recv(sock_file, buf, len, flags);
}

int syscall_unlink(const char* path, SYSCALL1) {
    if (!path) return -1;
    
    // Unlink socket
    return vfs_socket_unlink(path);
}

/* ================
   SYSCALL WRAPPER
   ================ */

int syscall(uint32_t syscall_id, int arg_count, ...) {
    uint32_t args[5] = {0};

    va_list ap;
    va_start(ap, arg_count);
    for (int i = 0; i < arg_count && i < 5; ++i) {
        args[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);

    int ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)                     // output: return value in eax
        : "a"(syscall_id),              // input: syscall number in eax
          "b"(args[0]),                 // arg1 in ebx
          "c"(args[1]),                 // arg2 in ecx
          "d"(args[2]),                 // arg3 in edx
          "S"(args[3]),                 // arg4 in esi
          "D"(args[4])                  // arg5 in edi
        : "memory"
    );

    return ret;
}

void* syscall_mmap(uintptr_t addr, size_t length, int prot, int flags, SYSCALL1) {
    proc_t* proc = get_current_process();
    if (!proc || !proc->vmspace) return NULL;
    
    printf("syscall_mmap: Mapping %u bytes at %p\n", length, addr);
    
    // Map memory in process's VM space
    if (vm_map_region(proc->vmspace, addr, 0, length, prot | VM_PROT_USER, flags) != 0) {
        return NULL;
    }
    
    return (void*)addr;
}

int syscall_win_create(const char* title, int x, int y, int width, int height) {
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    window_t* win = wm_create_window(proc->pid, title, x, y, width, height);
    if (!win) return -1;
    
    // Map window backbuffer physical pages into process's address space
    void* user_virt = (void*)(0x80000000 + (win->wid * 0x00400000)); // 4MB per window

    // Map each physical page from kernel buffer to user space
    vm_unmap_region(proc->vmspace, (uintptr_t)user_virt, win->buffer_size);
    // vm_copy_mappings(proc->vmspace,
    //                           (uintptr_t)win->backbuffer,
    //                           (uintptr_t)user_virt,
    //                           win->buffer_size,
    //                           VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER);
    
    return win->wid;
}

int syscall_win_destroy(uint32_t wid, SYSCALL1) {
    // Verify window belongs to calling process
    proc_t* proc = get_current_process();
    if (!proc) return -1;
    
    window_t* win = wm_get_window(wid);
    if (!win || win->pid != proc->pid) return -1;
    
    wm_destroy_window(wid);
    return 0;
}

int syscall_win_update(uint32_t wid, SYSCALL1) {
    // Verify window belongs to calling process
    proc_t* proc = get_current_process();
    if (!proc) return -1;

    window_t* win = wm_get_window(wid);
    if (!win || win->pid != proc->pid) return -1;
    
    wm_mark_dirty(wid);
    wm_composite();
    return 0;
}

void* syscall_win_getbuf(uint32_t wid, SYSCALL1) {
    // Verify window belongs to calling process
    proc_t* proc = get_current_process();
    if (!proc) return NULL;
    
    window_t* win = wm_get_window(wid);
    if (!win || win->pid != proc->pid) return NULL;
    
    // Return the virtual address where buffer is mapped (4MB per window)
    return (void*)(0x80000000 + (wid * 0x00400000));
}

int syscall_read_stdin(char* buffer, int count, SYSCALL1) {
    if (!buffer || count <= 0) return -1;
    
    int read_count = 0;
    
    // Read available characters up to count
    while (read_count < count && keyboard_has_input()) {
        char c = keyboard_getchar();
        if (c == 0) break;
        buffer[read_count++] = c;
    }
    
    // If nothing available, block properly (Linux-style wait queue)
    if (read_count == 0) {
        // Block this thread on the stdin wait queue
        // The keyboard interrupt will wake us up when input arrives
        block_current_thread(get_stdin_wait_queue());
        
        // When we wake up, input should be available
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            if (c != 0) {
                buffer[0] = c;
                read_count = 1;
            }
        }
    }
    
    return read_count;
}
