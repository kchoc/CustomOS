#include "stdio.h"
#include "string.h"
#include "syscalls.h"

int main()
{
    printf("Welcome to the kernel!\n");
    printf("This is a minimal shell. Type 'help' for a list of commands.\n");

    // Simple shell loop
    char  buf[128];
    char  program_path[64];
    char* argv[16];

    while (1) {
        memset(buf, 0, sizeof(buf));
        memset(program_path, 0, sizeof(program_path));
        memset(argv, 0, sizeof(argv));

        printf("> ");
        flush_stdout();
        fgets(buf, sizeof(buf), stdin);

        // Tokenize input (simple whitespace splitting) for argv
        int   argc     = 0;
        char* read_buf = buf;
        while (argc < 15 && read_buf[0] != '\n') {
            while (*read_buf == ' ' || *read_buf == '\t')
                read_buf++;

            if (read_buf[0] == '\n' || read_buf[0] == '\0')
                break; // End of input

            argv[argc++] = read_buf;

            while (read_buf[0] != ' ' && read_buf[0] != '\t' && read_buf[0] != '\n' &&
                   read_buf[0] != '\0')
                read_buf++; // Move to end of token
            if (read_buf[0])
                *read_buf++ = '\0'; // Null-terminate token and move to next
        }

        strcpy(program_path, "/bin/");
        strcat(program_path, argv[0]);
        strcat(program_path, ".elf");

        int fd = open(program_path, 0, 0);
        if (fd >= 0) {
            printf("Executing %s...\n", program_path);
            close(fd); // In a real shell, we would execve instead of just opening
        }
        else {
            printf("Command not found: %s\n", buf);
            continue;
        }

        if (fork() == 0) {
            // In child process
            execve(program_path, argv, NULL);
            printf("Failed to execute %s\n", program_path);
            exit(1);
        }
        printf("Started process for %s\n", program_path);
    }

    return 0;
}
