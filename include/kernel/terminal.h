#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

// Initialize the terminal
void terminal_initialize(void);

// Put a character on the terminal
void terminal_putchar(char c);

// Test in terminal for assembly
void terminal_test(void);

// Set the cursor position
void set_cursor_position(size_t row, size_t column);

// Input handler for the terminal
void terminal_input(uint8_t scancode, char c);

// Put a string on the terminal
void terminal_print(const char* str);

// Print an integer to the terminal
void terminal_print_int(int num);

// Print a hexadecimal number to the terminal
void terminal_print_hex(const void *object, size_t size);

// Clear the terminal screen
void terminal_clear(void);

// Handle backspace
void terminal_backspace(void);

// Terminal print register value
void terminal_print_register_value(const char* reg_name, uint32_t value);

#endif // TERMINAL_H