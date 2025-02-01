#include "kernel/commands.h"
#include "kernel/terminal.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/task.h"
#include "kernel/drivers/port_io.h"
#include "string.h"

void process_command(char *command) {
    char *args[10];
    int arg_count = 0;
    char *token = strtok(command, " ");
    while (token != NULL && arg_count < 10) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }

    if (strcmp(command, "help") == 0) {
        printf("Available commands: fizz, help, kmalloc, memory, pages, task, cpumode, reboot\n");
    } else if (strcmp(command, "fizz") == 0) {
        for (int i = 1; i <= 20; i++) {
            if (i % 3 == 0 && i % 5 == 0) {
                printf("FizzBuzz\n");
            } else if (i % 3 == 0) {
                printf("Fizz\n");
            } else if (i % 5 == 0) {
                printf("Buzz\n");
            } else {
                printf("%d\n", i);
            }
        }
    } else if (strcmp(command, "task") == 0) {
        if (strcmp(args[1], "list") == 0) {
            list_tasks();
        } else if (strcmp(args[1], "switch") == 0) {
            switch_task();
        } else if (strcmp(args[1], "help") == 0) {
            printf("Task options: help, list, start, switch\n");
        } else {
            printf("Usage: task <option>. Use 'task help' for options.\n");
        }
    } else if (strcmp(command, "kmalloc") == 0) {
        memory_usage();
    } else if (strcmp(command, "pages") == 0) {
        check_page_directory(current_page_directory, args[1] ? str2int(args[1]) : 0);
    } else if (strcmp(command, "memory") == 0) {
        int i = 0;
        printf("Current: %x\n\n", &i);
        check_page_directory(current_page_directory, args[1] ? str2int(args[1]) : 0);
        memory_usage();
        printf("\n");
    } else if (strcmp(command, "cpumode") == 0) {
        uint32_t mode;
        asm volatile("mov %%cr0, %0" : "=r" (mode));
        if (mode & 0x1) {
            printf("CPU is in protected mode\n");
        } else {
            printf("CPU is in real mode\n");
        }
    } else if (strcmp(command, "reboot") == 0) {
        printf("Rebooting...\n");
        outb(0x64, 0xFE); // Send reboot command to the keyboard controller
    } else {
        printf("Command not found: %s. Use help for a list of commands\n", command);
    }
}
