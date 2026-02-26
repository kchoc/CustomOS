#include "commands.h"
#include "machine/pmap.h"
#include "terminal.h"
#include "process.h"
#include "elf.h"
#include "pcpu.h"
#include "socket.h"

#include <dev/port/port_io.h>
#include <dev/video/vga.h>
#include <dev/video/vbe.h>

#include <fs/vfs.h>
#include <fs/sockfs.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <inttypes.h>
#include <string.h>

void process_command(char *input) {
    char *args[10];
    int arg_count = 0;

    // Tokenize the command input
    char *token = strtok(input, " ");
    while (token && arg_count < 10) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    arg_count--;

    if (arg_count == 0) return;

    const char *cmd = args[0];

    // Help
    if (strcmp(cmd, "help") == 0) {
        printf("Available commands:\n");
        printf("  help                 - Show this help message\n");
        printf("  kmalloc              - Display memory usage\n");
        printf("  memdump ADDR         - Dump memory contents\n");
        printf("  page ADDR            - Show page table entry for address\n");
        printf("  task                 - Manage tasks (list, switch, processes)\n");
        printf("  cpumode              - Display CPU mode (real/protected)\n");
        printf("  reboot               - Reboot the system\n");
        printf("  pagefault            - Trigger a page fault (for test)\n");
        printf("  syscall N            - Trigger software interrupt with syscall N\n");
        printf("  mounts               - List mounted filesystems\n");
        printf("  writef <file> <data> - Write string to file\n");
        printf("  readf <file>         - Read and print file content\n");
        printf("  mkdir <directory>    - Create a new directory\n");
        printf("  rmdir <directory>    - Remove an empty directory\n");
        printf("  create <file>        - Create a new empty file\n");
        printf("  ls <directory>       - List files in directory (default /)\n");
        printf("  rm <file>            - Remove a file\n");
        printf("  cd <directory>       - Change current directory\n");
        printf("  exec <binary>        - Execute a binary file\n");
        printf("  socktest             - Test sockfs functionality\n");
        printf("  socklist             - List all sockets in sockfs\n");
        printf("  gfxmode              - Switch to graphics mode (320x200)\n");
        printf("  textmode             - Switch back to text mode\n");
        return;
    }

    // Task commands
    if (strcmp(cmd, "task") == 0) {
        if (arg_count > 1 && strcmp(args[1], "list") == 0) {
            uint32_t cpu_id = arg_count > 2 ? (uint32_t)str2int(args[2]) : 0;
            list_pcpu_threads(&pcpus[cpu_id]);
        } else if (arg_count > 1 && strcmp(args[1], "processes") == 0) {
            list_tasks();
        } else {
            printf("Usage: task [processes|list (cpu|0)]\n");
        }
        return;
    }

    if (strcmp(cmd, "kmalloc") == 0) {
        memory_usage();
        return;
    }

    if (strcmp(cmd, "memdump") == 0) {
        if (arg_count < 2) {
            printf("Usage: memdump <address>\n");
            return;
        }
        uint32_t addr = str2int(args[1]);
        if (addr == 0) {
            printf("Invalid address: %s\n", args[1]);
            return;
        }
        printf("Memory dump at address %x:\n", addr);
        for (int i = 0; i < 4; i++) {
            printf("%x ", *((uint32_t *)(addr + i*4)));
        }
        printf("\n");
        return;
    }

    if (strcmp(cmd, "page") == 0) {
        if (arg_count < 2) {
            printf("Usage: page <address>\n");
            return;
        }
        uintptr_t addr = str2int(args[1]);
        if (addr == 0) {
            printf("Invalid address: %s\n", args[1]);
            return;
        }
        uint32_t phys = pmap_extract(&CURRENT_VM_SPACE->arch, addr);
        if (phys) {
            printf("Virtual address %x maps to physical address %x\n", addr, phys);
        } else {
            printf("Virtual address %x is not mapped\n", addr);
        }
        return;
    }

    if (strcmp(cmd, "cpumode") == 0) {
        uint32_t cr0;
        asm volatile("mov %%cr0, %0" : "=r"(cr0));
        printf("CPU is in %s mode\n", (cr0 & 0x1) ? "protected" : "real");
        return;
    }

    if (strcmp(cmd, "reboot") == 0) {
        printf("Rebooting...\n");
        outb(0x64, 0xFE);
        return;
    }

    if (strcmp(cmd, "pagefault") == 0) {
        int *ptr = (int *)0xC2000004;
        *ptr = 0;
        printf("Page fault triggered\n");
        return;
    }

    if (strcmp(cmd, "syscall") == 0) {
        if (arg_count > 1) {
            int syscall_num = str2int(args[1]);
            printf("Initiating syscall %d\n", syscall_num);
            asm volatile("int $0x80" : : "a"(syscall_num));
        } else {
            printf("Usage: syscall <number>\n");
        }
        return;
    }

    if (strcmp(cmd, "mounts") == 0) {
        vfs_print_mounts();
        return;
    }

    if (strcmp(cmd, "writef") == 0) {
        if (arg_count < 3) {
            printf("Usage: writef <filename> <data>\n");
            return;
        }
        const char *filename = args[1];
        const char *content = args[2];
        file_t *file = vfs_open(filename, 0x1 | 0x2 | 0x200, 0x8000); // Read-write, create if not exist
        if (!file) {
            printf("Failed to open file: %s\n", filename);
            return;
        }
        vfs_write(file, content, strlen(content), 0);
        vfs_close(file);
        printf("Wrote %uB to file: %s\n", strlen(content), filename);
        
        return;
    }

    if (strcmp(cmd, "readf") == 0) {
        if (arg_count < 2) {
            printf("Usage: readf <filename>\n");
            return;
        }

        const char *filename = args[1];
        file_t *file = vfs_open(filename, 0, 0x8000); // Read-only
        if (!file) {
            printf("Failed to open file: %s\n", filename);
            return;
        }


        char buffer[129];
        ssize_t bytes = vfs_read(file, buffer, 128, NULL);
        buffer[128] = '\0'; // Null-terminate
        printf("Read %d bytes from file: %s\n", bytes, filename);
        printf("%s\n", buffer);
        vfs_close(file);

        return;
    }

    if (strcmp(cmd, "mkdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: mkdir <directory>\n");
            return;
        }
        
        const char *path = args[1];
        const char *name = strrchr(path, '/');
        name = name ? name + 1 : path;
        if (strlen(name) > 11) {
            printf("Directory name too long (max 11 characters): %s\n", name);
            return;
        }
        dentry_t *dentry = vfs_lookup(NULL, path);
        if (dentry) {
            printf("Directory already exists: %s\n", path);
            return;
        }

        const char *parent_path = name == path ? "/" : strndup(path, name - path - 1);
        dentry_t *parent_dentry = vfs_lookup(NULL, parent_path);
        if (!parent_dentry || !parent_dentry->d_inode || !(parent_dentry->d_inode->i_mode & 0x4000)) {
            printf("Parent directory does not exist: %s\n", parent_path);
            return;
        }

        inode_t *parent_inode = parent_dentry->d_inode;
        dentry_t *new_dentry = alloc_dentry(name, NULL, parent_dentry);
        if (!new_dentry) {
            printf("Failed to allocate dentry for: %s\n", name);
            return;
        }
        if (vfs_mkdir(parent_inode, new_dentry, 0x4000) != 0) {
            printf("Failed to create directory: %s\n", path);
            kfree(new_dentry);
            return;
        }
        printf("Directory created: %s\n", path);

        return;
    }

    if (strcmp(cmd, "rmdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: rmdir <directory>\n");
            return;
        }
        const char *path = args[1];
        dentry_t *dentry = vfs_lookup(NULL, path);
        if (!dentry || !dentry->d_inode || !(dentry->d_inode->i_mode & 0x4000)) {
            printf("Directory does not exist: %s\n", path);
            return;
        }
        inode_t *parent_inode = dentry->d_parent ? dentry->d_parent->d_inode : NULL;
        if (!parent_inode) {
            printf("Cannot remove root directory\n");
            return;
        }
        if (vfs_rmdir(parent_inode, dentry) != 0) {
            printf("Failed to remove directory (not empty?): %s\n", path);
            return;
        }
        printf("Directory removed: %s\n", path);

        return;
    }

    if (strcmp(cmd, "create") == 0) {
        if (arg_count < 3) {
            printf("Usage: create <directory> <file>\n");
            return;
        }
        const char *dir_path = args[1];
        const char *file_name = args[2];
        if (strlen(file_name) > 11) {
            printf("File name too long (max 11 characters): %s\n", file_name);
            return;
        }

        dentry_t *dir_dentry = vfs_lookup(NULL, dir_path);
        if (!dir_dentry || !dir_dentry->d_inode || !(dir_dentry->d_inode->i_mode & 0x4000)) {
            printf("Directory does not exist: %s\n", dir_path);
            return;
        }
        inode_t *dir_inode = dir_dentry->d_inode;
        dentry_t *new_dentry = alloc_dentry(file_name, NULL, dir_dentry);
        if (!new_dentry) {
            printf("Failed to allocate dentry for: %s\n", file_name);
            return;
        }
        if (vfs_create(dir_inode, new_dentry, 0x8000, false) != 0) {
            printf("Failed to create file: %s/%s\n", dir_path, file_name);
            kfree(new_dentry);
            return;
        }
        printf("File created: %s/%s\n", dir_path, file_name);
        return;
    }

    if (strcmp(cmd, "ls") == 0) {
        if (arg_count < 2) {
            printf("Usage: ls <directory>\n");
            return;
        }

        const char *path = args[1];
        vfs_ls(path);

        return;
    }

    if (strcmp(cmd, "rm") == 0) {
        if (arg_count < 2) {
            printf("Usage: rm <file>\n");
            return;
        }
        return;
    }

    if (strcmp(cmd, "cd") == 0) {
        if (arg_count < 2) {
            printf("Usage: cd <directory>\n");
            return;
        }
        return;
    }

    if (strcmp(cmd, "exec") == 0) {
        if (arg_count < 2) {
            printf("Usage: exec <binary>\n");
            return;
        }
        create_process_from_elf(args[1]);
        return;
    }

    if (strcmp(cmd, "socktest") == 0) {
        printf("=== Sockfs Test ===\n");
        
        // Create a socket
        printf("Creating socket 'test_sock'...\n");
        if (vfs_socket_create("test_sock", SOCK_TYPE_STREAM, 0666)) {
            printf("Failed to create socket\n");
            return;
        }
        printf("Socket created successfully\n");
        
        // Lookup the socket
        dentry_t* sock_dentry = sockfs_lookup_socket("test_sock");
        if (!sock_dentry) {
            printf("Failed to lookup socket\n");
            return;
        }
        printf("Socket found in sockfs\n");
        
        // Make it a listening socket
        const socket_ops_t* sock_ops = sockfs_get_socket_ops();
        if (sock_ops->listen(sock_dentry, 5)) {
            printf("Failed to listen on socket\n");
            return;
        }
        printf("Socket is now listening\n");
        
        // Create a client socket
        printf("Creating client socket...\n");
        if (vfs_socket_create("client_sock", SOCK_TYPE_STREAM, 0666)) {
            printf("Failed to create client socket\n");
            return;
        }
        
        // Connect to the server
        printf("Connecting to test_sock...\n");
        file_t* client_file = vfs_socket_connect("test_sock", 0);
        if (!client_file) {
            printf("Failed to connect\n");
            return;
        }
        printf("Connected successfully\n");
        
        // Send a message
        const char* msg = "Hello, Socket!";
        printf("Sending message: %s\n", msg);
        ssize_t sent = vfs_socket_send(client_file, msg, strlen(msg), 0);
        if (sent < 0) {
            printf("Failed to send message\n");
        } else {
            printf("Sent %d bytes\n", sent);
        }
        
        // Try to receive (should have data if peer socket works)
        char buffer[64];
        ssize_t received = vfs_socket_recv(client_file, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            printf("Received: %s\n", buffer);
        } else {
            printf("No data received (expected for now)\n");
        }
        
        // Close client
        vfs_close(client_file);
        printf("Client closed\n");
        
        // Cleanup
        vfs_socket_unlink("test_sock");
        vfs_socket_unlink("client_sock");
        printf("Sockets cleaned up\n");
        printf("=== Test Complete ===\n");
        return;
    }

    if (strcmp(cmd, "socklist") == 0) {
        sockfs_list_sockets();
        return;
    }

    if (strcmp(cmd, "gfxmode") == 0) {
        printf("Switching to graphics mode (320x200)...\n");
        vga_set_mode_13h();
        printf("Graphics mode enabled. Use 'textmode' to return.\n");
        return;
    }

    if (strcmp(cmd, "textmode") == 0) {
        vga_set_mode_text();
        terminal_init();
        printf("Text mode restored.\n");
        return;
    }

    printf("Unknown command: %s. Type 'help' for a list of commands.\n", cmd);
}
