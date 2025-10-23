#include "kernel/drivers/port_io.h"
#include "kernel/terminal.h"
#include "kernel/commands.h"
#include "kernel/memory/layout.h"
#include "types/string.h"
#include "types/bool.h"

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

static void reverse(char *str, int len) {
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
}

// helper: convert integer to string
static void itoa_base(uint32_t value, char *buf, int base, bool uppercase) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value) {
            buf[i++] = digits[value % base];
            value /= base;
        }
    }
    buf[i] = '\0';
    reverse(buf, i);
}

// helper: print integer with optional padding
static void print_uint(uint32_t val, int base, int width, char pad_char, bool uppercase) {
    char buf[33]; // Enough for 32-bit binary + null
    itoa_base(val, buf, base, uppercase);
    int len = strlen(buf);
    for (int i = len; i < width; i++) {
        terminal_putchar(pad_char);
    }
    terminal_print(buf);
}

static void print_int(int32_t val, int width, char pad_char) {
    if (val < 0) {
        terminal_putchar('-');
        val = -val;
        width--;
    }
    print_uint((uint32_t)val, 10, width, pad_char, false);
}

// Main printf
void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            terminal_putchar(*fmt);
            continue;
        }

        fmt++;

        // handle %%
        if (*fmt == '%') {
            terminal_putchar('%');
            continue;
        }

        // parse padding
        char pad_char = ' ';
        int width = 0;
        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // format specifiers
        switch (*fmt) {
            case 'd':
            case 'i': {
                int32_t v = va_arg(args, int32_t);
                print_int(v, width, pad_char);
                break;
            }
            case 'u': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(v, 10, width, pad_char, false);
                break;
            }
            case 'x': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(v, 16, width, pad_char, false);
                break;
            }
            case 'X': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(v, 16, width, pad_char, true);
                break;
            }
            case 'p': {
                uintptr_t ptr = (uintptr_t)va_arg(args, void *);
                terminal_print("0x");
                print_uint(ptr, 16, width ? width : (int)(sizeof(void*) * 2), '0', false);
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char *);
                if (!str) str = "(null)";
                terminal_print(str);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            default:
                terminal_putchar('%');
                terminal_putchar(*fmt);
                break;
        }
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
    printf("Register value: %x\n", value);
    delay(400);
}

void terminal_print_registers(registers_t* regs) {
    printf("Registers:\n");
    printf("EAX: %x  EBX: %x  ECX: %x  EDX: %x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("ESI: %x  EDI: %x  EBP: %x  ESP: %x\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    printf("EIP: %x  CS: %x  EFLAGS: %x\n", regs->eip, regs->cs, regs->eflags);
    printf("DS: %x  ES: %x  FS: %x  GS: %x\n", regs->ds, regs->es, regs->fs, regs->gs);
}

void delay(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (uint32_t j = 0; j < 1000000; j++) {}
    }
}
