#ifndef TERMINAL_H
#define TERMINAL_H

#include "kernel/types.h"
#include "types/common.h"

// Initialize the terminal
void terminal_init(void);

// Put a character on the terminal
void terminal_putchar(char c);

// Test in terminal for assembly
void terminal_test(void);

// Set the cursor position
void set_cursor_position(size_t row, size_t column);

// Print a formatted string to the terminal
void printf(const char *s, ...);

// Input handler for the terminal
void terminal_input(uint8_t scancode, char c);

// Put a string on the terminal
void terminal_print(const char *str);

// Clear the terminal screen
void terminal_clear(void);

// Handle backspace
void terminal_backspace(void);

// Terminal print register value
void terminal_print_register_value(uint32_t value);

// Print registers
void terminal_print_registers(registers_t* regs);

// Delay function
void delay(uint32_t ms);

#endif // TERMINAL_H