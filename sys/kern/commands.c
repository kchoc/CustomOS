// #include "commands.h"
// #include "terminal.h"
// #include "process.h"
// #include "elf.h"
// #include "socket.h"
//
// #include <dev/port/port_io.h>
// #include <dev/video/vga.h>
// #include <dev/video/vbe.h>
//
// #include <sys/pcpu.h>
//
// #include <fs/vfs.h>
// #include <fs/sockfs.h>
//
// #include <vm/kmalloc.h>
// #include <vm/vm_map.h>
//
// #include <machine/pmap.h>
//
// #include <inttypes.h>
// #include <string.h>
//
// void process_command(char *input) {
//     char *args[10];
//     int arg_count = 0;
//
//     // Tokenize the command input
//     char *token = strtok(input, " ");
//     while (token && arg_count < 10) {
//         args[arg_count++] = token;
//         token = strtok(NULL, " ");
//     }
//     arg_count--;
//
//     if (arg_count == 0) return;
//
//     const char *cmd = args[0];
//
//     // Help
//     if (strcmp(cmd, "help") == 0) {
//         printf("Available commands:\n");
//         printf("  help                 - Show this help message\n");
//         printf("  kmalloc              - Display memory usage\n");
//         printf("  memdump ADDR         - Dump memory contents\n");
//         printf("  page ADDR            - Show page table entry for address\n");
//         printf("  task                 - Manage tasks (list, switch, processes)\n");
//         printf("  cpumode              - Display CPU mode (real/protected)\n");
//         printf("  reboot               - Reboot the system\n");
//         printf("  pagefault            - Trigger a page fault (for test)\n");
//         printf("  syscall N            - Trigger software interrupt with syscall N\n");
//         printf("  mounts               - List mounted filesystems\n");
//         printf("  writef <file> <data> - Write string to file\n");
//         printf("  readf <file>         - Read and print file content\n");
//         printf("  mkdir <directory>    - Create a new directory\n");
//         printf("  rmdir <directory>    - Remove an empty directory\n");
//         printf("  create <file>        - Create a new empty file\n");
//         printf("  ls <directory>       - List files in directory (default /)\n");
//         printf("  rm <file>            - Remove a file\n");
//         printf("  cd <directory>       - Change current directory\n");
//         printf("  exec <binary>        - Execute a binary file\n");
//         printf("  socktest             - Test sockfs functionality\n");
//         printf("  socklist             - List all sockets in sockfs\n");
//         printf("  gfxmode              - Switch to graphics mode (320x200)\n");
//         printf("  textmode             - Switch back to text mode\n");
//         return;
//     }
//
//     // Task commands
//     if (strcmp(cmd, "task") == 0) {
//         if (arg_count > 1 && strcmp(args[1], "list") == 0) {
//             uint32_t cpu_id = arg_count > 2 ? (uint32_t)str2int(args[2]) : 0;
//             list_pcpu_threads(&pcpus[cpu_id]);
//         } else if (arg_count > 1 && strcmp(args[1], "processes") == 0) {
//             list_tasks();
//         } else {
//             printf("Usage: task [processes|list (cpu|0)]\n");
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "kmalloc") == 0) {
//         memory_usage();
//         return;
//     }
//
//     if (strcmp(cmd, "memdump") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: memdump <address>\n");
//             return;
//         }
//         uint32_t addr = str2int(args[1]);
//         if (addr == 0) {
//             printf("Invalid address: %s\n", args[1]);
//             return;
//         }
//         printf("Memory dump at address %x:\n", addr);
//         for (int i = 0; i < 4; i++) {
//             printf("%x ", *((uint32_t *)(addr + i*4)));
//         }
//         printf("\n");
//         return;
//     }
//
//     if (strcmp(cmd, "page") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: page <address>\n");
//             return;
//         }
//         uintptr_t addr = str2int(args[1]);
//         if (addr == 0) {
//             printf("Invalid address: %s\n", args[1]);
//             return;
//         }
//         uint32_t phys = pmap_extract(kernel_vm_space->arch, addr);
//         if (phys) {
//             printf("Virtual address %x maps to physical address %x\n", addr, phys);
//         } else {
//             printf("Virtual address %x is not mapped\n", addr);
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "cpumode") == 0) {
//         uint32_t cr0;
//         asm volatile("mov %%cr0, %0" : "=r"(cr0));
//         printf("CPU is in %s mode\n", (cr0 & 0x1) ? "protected" : "real");
//         return;
//     }
//
//     if (strcmp(cmd, "reboot") == 0) {
//         printf("Rebooting...\n");
//         outb(0x64, 0xFE);
//         return;
//     }
//
//     if (strcmp(cmd, "pagefault") == 0) {
//         int *ptr = (int *)0xC2000004;
//         *ptr = 0;
//         printf("Page fault triggered\n");
//         return;
//     }
//
//     if (strcmp(cmd, "syscall") == 0) {
//         if (arg_count > 1) {
//             int syscall_num = str2int(args[1]);
//             printf("Initiating syscall %d\n", syscall_num);
//             asm volatile("int $0x80" : : "a"(syscall_num));
//         } else {
//             printf("Usage: syscall <number>\n");
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "mounts") == 0) {
//         vfs_print_mounts();
//         return;
//     }
//
//     if (strcmp(cmd, "writef") == 0) {
//         if (arg_count < 3) {
//             printf("Usage: writef <filename> <data>\n");
//             return;
//         }
//         const char *filename = args[1];
//         const char *content = args[2];
//         file_t *file = vfs_open(filename, 0x1 | 0x2 | 0x200, 0x8000); // Read-write, create if
//         not exist if (!file) {
//             printf("Failed to open file: %s\n", filename);
//             return;
//         }
//         vfs_write(file, content, strlen(content), 0);
//         vfs_close(file);
//         printf("Wrote %uB to file: %s\n", strlen(content), filename);
//
//         return;
//     }
//
//     if (strcmp(cmd, "readf") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: readf <filename>\n");
//             return;
//         }
//
//         const char *filename = args[1];
//         file_t *file = vfs_open(filename, 0, 0x8000); // Read-only
//         if (!file) {
//             printf("Failed to open file: %s\n", filename);
//             return;
//         }
//
//
//         char buffer[129];
//         ssize_t bytes = vfs_read(file, buffer, 128, NULL);
//         buffer[128] = '\0'; // Null-terminate
//         printf("Read %d bytes from file: %s\n", bytes, filename);
//         printf("%s\n", buffer);
//         vfs_close(file);
//
//         return;
//     }
//
//     if (strcmp(cmd, "ls") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: ls <directory>\n");
//             return;
//         }
//
//         const char *path = args[1];
//         vfs_ls(path);
//
//         return;
//     }
//
//     if (strcmp(cmd, "rm") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: rm <file>\n");
//             return;
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "cd") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: cd <directory>\n");
//             return;
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "exec") == 0) {
//         if (arg_count < 2) {
//             printf("Usage: exec <binary>\n");
//             return;
//         }
//         return;
//     }
//
//     if (strcmp(cmd, "gfxmode") == 0) {
//         printf("Switching to graphics mode (320x200)...\n");
//         vga_set_mode_13h();
//         printf("Graphics mode enabled. Use 'textmode' to return.\n");
//         return;
//     }
//
//     if (strcmp(cmd, "textmode") == 0) {
//         vga_set_mode_text();
//         terminal_init();
//         printf("Text mode restored.\n");
//         return;
//     }
//
//     printf("Unknown command: %s. Type 'help' for a list of commands.\n", cmd);
// }
