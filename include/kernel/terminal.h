#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

// Initialize the terminal
void terminal_initialize(void);

// Put a character on the terminal
void terminal_putchar(char c);

// Put a string on the terminal
void terminal_print(const char* str);

// Clear the terminal screen
void terminal_clear(void);

// Handle backspace
void terminal_backspace(void);

#endif // TERMINAL_H