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

// Output context for formatting functions
typedef struct {
    char *buffer;       // For sprintf/snprintf: buffer pointer
    size_t buffer_size; // For snprintf: max size
    size_t written;     // Number of characters written
    bool to_buffer;     // true = write to buffer, false = write to terminal
} output_ctx_t;

// Output a single character to the context
static void output_char(output_ctx_t *ctx, char c) {
    if (ctx->to_buffer) {
        // Write to buffer if there's space (leave room for null terminator)
        if (ctx->buffer_size == 0 || ctx->written < ctx->buffer_size - 1) {
            ctx->buffer[ctx->written] = c;
        }
    } else {
        // Write to terminal
        terminal_putchar(c);
    }
    ctx->written++;
}

// Output a string to the context
static void output_string(output_ctx_t *ctx, const char *str) {
    while (*str) {
        output_char(ctx, *str++);
    }
}

// helper: print integer with optional padding
static void print_uint(output_ctx_t *ctx, uint32_t val, int base, int width, char pad_char, bool uppercase) {
    char buf[33]; // Enough for 32-bit binary + null
    itoa_base(val, buf, base, uppercase);
    int len = strlen(buf);
    for (int i = len; i < width; i++) {
        output_char(ctx, pad_char);
    }
    output_string(ctx, buf);
}

static void print_int(output_ctx_t *ctx, int32_t val, int width, char pad_char) {
    if (val < 0) {
        output_char(ctx, '-');
        val = -val;
        width--;
    }
    print_uint(ctx, (uint32_t)val, 10, width, pad_char, false);
}

// Internal formatting function used by printf, sprintf, and snprintf
static int vformat(output_ctx_t *ctx, const char *fmt, va_list args) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            output_char(ctx, *fmt);
            continue;
        }

        fmt++;

        // handle %%
        if (*fmt == '%') {
            output_char(ctx, '%');
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
                print_int(ctx, v, width, pad_char);
                break;
            }
            case 'u': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(ctx, v, 10, width, pad_char, false);
                break;
            }
            case 'x': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(ctx, v, 16, width, pad_char, false);
                break;
            }
            case 'X': {
                uint32_t v = va_arg(args, uint32_t);
                print_uint(ctx, v, 16, width, pad_char, true);
                break;
            }
            case 'p': {
                uintptr_t ptr = (uintptr_t)va_arg(args, void *);
                output_string(ctx, "0x");
                print_uint(ctx, ptr, 16, width ? width : (int)(sizeof(void*) * 2), '0', false);
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char *);
                if (!str) str = "(null)";
                output_string(ctx, str);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                output_char(ctx, c);
                break;
            }
            default:
                output_char(ctx, '%');
                output_char(ctx, *fmt);
                break;
        }
    }

    // Null-terminate if writing to buffer
    if (ctx->to_buffer && ctx->buffer_size > 0) {
        size_t pos = ctx->written < ctx->buffer_size ? ctx->written : ctx->buffer_size - 1;
        ctx->buffer[pos] = '\0';
    }

    return (int)ctx->written;
}

// Main printf
void printf(const char *fmt, ...) {
    output_ctx_t ctx = {
        .buffer = NULL,
        .buffer_size = 0,
        .written = 0,
        .to_buffer = false
    };

    va_list args;
    va_start(args, fmt);
    vformat(&ctx, fmt, args);
    va_end(args);
    
    set_cursor_position(terminal_row, terminal_column);
}

// sprintf - write formatted string to buffer (no size limit)
int sprintf(char *buffer, const char *fmt, ...) {
    if (!buffer) return -1;

    output_ctx_t ctx = {
        .buffer = buffer,
        .buffer_size = 0, // No limit for sprintf
        .written = 0,
        .to_buffer = true
    };

    va_list args;
    va_start(args, fmt);
    int ret = vformat(&ctx, fmt, args);
    va_end(args);

    // Always null-terminate
    buffer[ctx.written] = '\0';
    
    return ret;
}

// snprintf - write formatted string to buffer with size limit
int snprintf(char *buffer, size_t size, const char *fmt, ...) {
    if (!buffer || size == 0) return -1;

    output_ctx_t ctx = {
        .buffer = buffer,
        .buffer_size = size,
        .written = 0,
        .to_buffer = true
    };

    va_list args;
    va_start(args, fmt);
    int ret = vformat(&ctx, fmt, args);
    va_end(args);

    return ret;
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
