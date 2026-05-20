#include "stdio.h"
#include "string.h"
#include "syscalls.h"

int main() {
    printf("Welcome to the kernel!\n");
    printf("This is a minimal shell. Type 'help' for a list of commands.\n");

    // Simple shell loop
    char buf[128];
    while (1) {
        printf("> ");
        flush_stdout(); 
        int n = scanf("%s", buf);
        if (n <= 0) {
            printf("Error reading input\n");
            continue;
        }
        if (strlen(buf) > 50) {
            printf("Input too long\n");
            continue;
        }

        char program_path[64];
        strcpy(program_path, "/sbin/");
        strcat(program_path, buf);
        strcat(program_path, ".elf");

        int fd = open(program_path, 0, 0);
        if (fd >= 0) {
            printf("Executing %s...\n", program_path);
            close(fd); // In a real shell, we would execve instead of just opening
        } else {
            printf("Command not found: %s\n", buf);
            continue;
        }

        if (fork() == 0) {
            // In child process
            char* args[] = {program_path, NULL};
            execve(program_path, args, NULL);
            printf("Failed to execute %s\n", program_path);
            exit(1);
        }
        printf("Started process for %s\n", program_path);

        while (1) asm volatile("nop");
    }

    return 0;
}


