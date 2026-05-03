#include "stdio.h"
#include "syscalls.h"

int _start() {
    printf("List of Available Commands:\n");
    printf("  help - Display this help message\n");
    printf("  echo [message] - Print the provided message\n");
    printf("  exit - Exit the shell\n");
    exit(0);
    return 0;
}
