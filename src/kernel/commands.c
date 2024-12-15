#include "kernel/commands.h"
#include "kernel/terminal.h"
#include "string.h"

void process_command(const char *command) {
    if (strcmp(command, "help") == 0) {
        terminal_print("Available commands: help, clear\n");
    } else if (strcmp(command, "clear") == 0) {
        terminal_clear();
    } else {
        terminal_print("Unknown command\n");
    }
}
