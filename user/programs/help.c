#include "stdio.h"
#include "syscalls.h"

int main() {
    printf("List of Available Commands:\n");
    printf("  help - Display this help message\n");
    printf("  ls - List files in the current directory\n"); 
    printf("  exit - Exit the shell\n");

    while(1) {
        asm volatile("nop"); // Keep the shell running until the user exits
    }
    return 0;
}
