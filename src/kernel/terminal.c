#include "kernel/drivers/port_io.h"
#include "kernel/terminal.h"
#include "kernel/commands.h"
#include "kernel/memory/layout.h"
#include "types/string.h"
#include "itoa.h"

#include <stddef.h>
#include <stdarg.h>

// Global terminal variables
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint16_t *video_memory = (uint16_t *)VGA_ADDRESS;
static uint8_t command_mode = 0;

// Initialize the terminal
void terminal_init() {
    terminal_clear();
}

// Set the cursor position
void set_cursor_position(size_t row, size_t column) {
    uint16_t position = (row * 80) + column;

    // Send the high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);

    // Send the low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
}

// Put a character on the terminal
void terminal_putchar(char c) {
    if (c == '\n') {
        for (size_t i = terminal_column + terminal_row * 80; i < terminal_row * 80 + 80; i++) {
            video_memory[i] = ' ' | 0x0700;
        }
        terminal_row++;
        terminal_column = 0;
    } else {
        const size_t index = terminal_row * 80 + terminal_column;
        if (index >= 1920) {
            return;
        }
        video_memory[index] = (uint16_t)c | 0x0700; // White on black
        terminal_column++;
    }

    if (terminal_column >= 80) {
        terminal_column = 0;
        terminal_row++;
    }

    if (terminal_row >= 25) {
        terminal_row = 0;
    }
}

// Test in terminal for assembly
void terminal_test(void) {
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    printf("test esp: %x\n", esp);
    delay(200);
}

// Print a formatted string to the terminal
void printf(const char *s, ...) {
    va_list args;

    uint32_t u;
    int32_t i;
    char *str;

    va_start(args, s);

    while (*s) {
        if (*s != '%') {
            terminal_putchar(*s++);
            continue;
        }
        s++;
        switch (*s) {
            case 'd':
                i = va_arg(args, int32_t);
                terminal_print_int(i);
                break;
            case 'u':
                u = va_arg(args, uint32_t);
                terminal_print_int(u);
                break;
            case 'x':
                u = va_arg(args, uint32_t);
                terminal_print_hex(&u, sizeof(u));
                break;
            case 's':
                str = va_arg(args, char *);
                terminal_print(str);
                break;
            case 'c':
                u = va_arg(args, int32_t);
                terminal_putchar(u);
                break;
            case 0:
                return;
            default:
                terminal_putchar(*s);
                break;
        }
        s++;
    }
    va_end(args);
    set_cursor_position(terminal_row, terminal_column);
}

// Input handler for the terminal
void terminal_input(uint8_t scancode, char c) {
    if (!command_mode) {
		if (c == '\n') {
			command_mode = 1;
			terminal_clear();
			terminal_putchar('>');
		}
        return;
    }
    if (c == '\b')
        terminal_backspace();
    else if (c == 0) {
        if (scancode == 0x48) { // Up Arrow
            if (terminal_row > 0) {
                terminal_row--;
            }
        } else if (scancode == 0x50) { // Down Arrow
            if (terminal_row < 24) {
                terminal_row++;
            }
        } else if (scancode == 0x4B) { // Left Arrow
            if (terminal_column > 0) {
                terminal_column--;
            }
        } else if (scancode == 0x4D) { // Right Arrow
            if (terminal_column < 79) {
                terminal_column++;
            }
        }
    } else if (c == '\n') {
        char command_buffer[160] = { 0 };
		for (int i = 0; i < 160; i++) {
			command_buffer[i] = (char)(video_memory[i + 1] & 0x00FF);
		}
		command_mode = 0;
		terminal_clear();
		process_command(command_buffer);
	}
	else {
		terminal_putchar(c);
	}
    set_cursor_position(terminal_row, terminal_column);
}

// Put a string on the terminal
void terminal_print(const char *str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}

// Print an integer to the terminal
void terminal_print_int(int num) {
    char str[16];
    itoa(num, str, 10);
    terminal_print(str);
}

// Print a hexadecimal number to the terminal
void terminal_print_hex(const void *object, size_t size) {
    const uint8_t *byte = (const uint8_t *)object;
    char str[size * 2 + 3];
    str[0] = '0';
    str[1] = 'x';
    for (size_t i = 0; i < size; i++) {
        uint8_t value = byte[size - 1 - i];
        str[2 + i * 2] = (value >> 4) < 10 ? '0' + (value >> 4) : 'A' + (value >> 4) - 10;
        str[3 + i * 2] = (value & 0xF) < 10 ? '0' + (value & 0xF) : 'A' + (value & 0xF) - 10;
    }
    str[size * 2 + 2] = '\0';
    terminal_print(str);
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

void terminal_print_register_value(uint32_t value) {
    terminal_print("Register Value: ");
    terminal_print_hex(&value, sizeof(value));
    terminal_print("\n");

    delay(400);
}

void delay(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (uint32_t j = 0; j < 1000000; j++) {}
    }
}
