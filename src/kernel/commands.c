#include "kernel/commands.h"
#include "kernel/terminal.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/process/process.h"
#include "kernel/process/elf.h"
#include "kernel/drivers/port_io.h"
#include "kernel/filesystem/fs.h"

#include "types/string.h"
#include <stdint.h>

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
        printf("  task                 - Manage tasks (list, switch)\n");
        printf("  cpumode              - Display CPU mode (real/protected)\n");
        printf("  reboot               - Reboot the system\n");
        printf("  pagefault            - Trigger a page fault (for test)\n");
        printf("  syscall N            - Trigger software interrupt with syscall N\n");
        printf("  writef <file> <data> - Write string to file\n");
        printf("  readf <file>         - Read and print file content\n");
        printf("  mkdir <directory>    - Create a new directory\n");
        printf("  ls <directory>       - List files in directory (default /)\n");
        printf("  rm <file>            - Remove a file\n");
        printf("  path <path>          - Tests path parsing\n");
        printf("  cd <directory>       - Change current directory\n");
        printf("  exec <binary>        - Execute a binary file\n");
        return;
    }

    // Task commands
    if (strcmp(cmd, "task") == 0) {
        if (arg_count > 1 && strcmp(args[1], "switch") == 0) {
            schedule();
        } else if (arg_count > 1 && strcmp(args[1], "list") == 0) {
            list_tasks();
        } else {
            printf("Usage: task [list|switch]\n");
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

    if (strcmp(cmd, "writef") == 0) {
        if (arg_count < 3) {
            printf("Usage: writef <filename> <data>\n");
            return;
        }
        const char *filename = args[1];
        const char *content = args[2];
        fat16_write_file(filename, (const uint8_t *)content, strlen(content));
        printf("Wrote to %s\n", filename);
        return;
    }

    if (strcmp(cmd, "readf") == 0) {
        if (arg_count < 2) {
            printf("Usage: readf <filename>\n");
            return;
        }

        const char *filename = args[1];
        uint8_t *buf;
        uint32_t size;
        fat16_read_file(filename, &buf, &size);
        buf[4095] = '\0'; // Null-terminate
        printf("%s\n", buf);
        kfree(buf);
        return;
    }

    if (strcmp(cmd, "mkdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: mkdir <directory>\n");
            return;
        }
        if (fat16_create_dir(args[1]) == 0) {
            printf("Directory created: %s\n", args[1]);
        } else {
            printf("Failed to create directory: %s\n", args[1]);
        }
        return;
    }

    if (strcmp(cmd, "ls") == 0) {
        fat16_list_dir();
        return;
    }

    if (strcmp(cmd, "rm") == 0) {
        if (arg_count < 2) {
            printf("Usage: rm <file>\n");
            return;
        }
        if (fat16_delete_file(args[1]) == 0) {
            printf("File deleted: %s\n", args[1]);
        } else {
            printf("Failed to delete file: %s\n", args[1]);
        }
        return;
    }

    if (strcmp(cmd, "path") == 0) {
        if (arg_count < 2) {
            printf("Usage: path <path>\n");
            return;
        }
        path_t *parsed_path = parse_path(args[1]);
        if (parsed_path) {
            while (parsed_path) {
                printf("%s/", parsed_path->name);
                parsed_path = parsed_path->subdir;
            }
            printf("\n");
            
            destroy_path(parsed_path);
        } else {
            printf("Invalid path format.\n");
        }
        return;
    }

    if (strcmp(cmd, "cd") == 0) {
        if (arg_count < 2) {
            printf("Usage: cd <directory>\n");
            return;
        }
        if (fat16_change_directory(args[1]) == 0) {
            printf("Changed directory to: %s\n", args[1]);
        } else {
            printf("Failed to change directory to: %s\n", args[1]);
        }
        return;
    }

    if (strcmp(cmd, "exec") == 0) {
        if (arg_count < 2) {
            printf("Usage: exec <binary>\n");
            return;
        }
        create_process_from_elf(args[1]);
        printf("Executing Task!\n");
        yeild();
        return;
    }


    printf("Unknown command: %s. Type 'help' for a list of commands.\n", cmd);
}
