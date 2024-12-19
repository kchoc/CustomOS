#include "kernel/commands.h"
#include "kernel/terminal.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
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
        printf("Available commands: help, fizz, kmalloc, pages\n");
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
    } else if (strcmp(command, "kmalloc") == 0) {
        memory_usage();
    } else if (strcmp(command, "pages") == 0) {
        show_pages();
    } else {
        printf("Command not found: %s. Use help for a list of commands\n", command);
    }
}
