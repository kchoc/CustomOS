#include "kernel/terminal.h"
#include "string.h"
#include <stddef.h>

// Global terminal variables
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint16_t *video_memory = (uint16_t *)0xB8000;

// Initialize the terminal
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_clear();
}

// Put a character on the terminal
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_row++;
        terminal_column = 0;
    } else {
        const size_t index = terminal_row * 80 + terminal_column;
        video_memory[index] = (uint16_t)c | 0x0700; // White on black
        terminal_column++;
    }

    if (terminal_column >= 80) {
        terminal_column = 0;
        terminal_row++;
    }
}

// Put a string on the terminal
void terminal_print(const char *str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}

// Clear the terminal screen
void terminal_clear(void) {
    for (size_t i = 0; i < 2000; i++) {
        video_memory[i] = ' ' | 0x0700;
    }
    terminal_row = 0;
    terminal_column = 0;
}

// Handle backspace
void terminal_backspace(void) {
    if (terminal_column > 0) {
        terminal_column--;
    } else if (terminal_row > 0) {
        terminal_row--;
        terminal_column = 79;
    }
    size_t index = terminal_row * 80 + terminal_column;
    video_memory[index] = ' ' | 0x0700;
}
