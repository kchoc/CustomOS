#include "stdio.h"
#include "syscalls.h"
#include "string.h"

/* Input buffer (filled by read_stdin syscall) */
#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static int input_pos = 0;
static int input_size = 0;

/* Output buffer (flushed by write syscall) */
#define OUTPUT_BUFFER_SIZE 256
static char output_buffer[OUTPUT_BUFFER_SIZE];
static int output_pos = 0;

/* Flush output buffer to stdout */
void flush_stdout(void) {
    if (output_pos > 0) {
        print(output_buffer);  // Use existing print syscall
        output_pos = 0;
    }
}

/* Buffered getchar - reads from input buffer, refills with syscall when empty */
int getchar(void) {
    if (input_pos >= input_size) {
        // Refill buffer with blocking read
        input_size = read_stdin(input_buffer, INPUT_BUFFER_SIZE);
        input_pos = 0;
        if (input_size <= 0) {
            return EOF;
        }
    }
    return (unsigned char)input_buffer[input_pos++];
}

/* Buffered putchar - writes to output buffer, flushes when full or on newline */
int putchar(int c) {
    if (output_pos >= OUTPUT_BUFFER_SIZE) {
        flush_stdout();
    }
    
    output_buffer[output_pos++] = (char)c;
    
    // Auto-flush on newline (line-buffered)
    if (c == '\n') {
        flush_stdout();
    }
    
    return c;
}

/* Write string to stdout */
int puts(const char* s) {
    if (!s) return EOF;
    
    while (*s) {
        putchar(*s++);
    }
    putchar('\n');
    flush_stdout();
    return 0;
}

/* Simple printf implementation */
int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

/* Number to string conversion helper */
static void print_number(unsigned long n, int base, int is_signed, int width) {
    char buffer[32];
    int i = 0;
    int is_negative = 0;
    
    if (is_signed && (long)n < 0) {
        is_negative = 1;
        n = -(long)n;
    }
    
    if (n == 0) {
        buffer[i++] = '0';
    } else {
        while (n > 0) {
            int digit = n % base;
            buffer[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            n /= base;
        }
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    // Pad with spaces
    while (i < width) {
        buffer[i++] = ' ';
    }
    
    // Print in reverse
    while (i > 0) {
        putchar(buffer[--i]);
    }
}

/* vprintf - format string processor */
int vprintf(const char* format, va_list args) {
    int count = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            int width = 0;
            
            // Parse width
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
            
            switch (*format) {
                case 'd':
                case 'i': {
                    int n = va_arg(args, int);
                    print_number(n, 10, 1, width);
                    break;
                }
                case 'u': {
                    unsigned int n = va_arg(args, unsigned int);
                    print_number(n, 10, 0, width);
                    break;
                }
                case 'x': {
                    unsigned int n = va_arg(args, unsigned int);
                    print_number(n, 16, 0, width);
                    break;
                }
                case 'p': {
                    void* p = va_arg(args, void*);
                    putchar('0');
                    putchar('x');
                    print_number((unsigned long)p, 16, 0, 8);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s) {
                        putchar(*s++);
                        count++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case '%':
                    putchar('%');
                    count++;
                    break;
                default:
                    putchar('%');
                    putchar(*format);
                    count += 2;
                    break;
            }
            format++;
        } else {
            putchar(*format++);
            count++;
        }
    }
    
    return count;
}

/* File descriptor functions */
int fputc(int c, int fd) {
    char buf = (char)c;
    return write(fd, &buf, 1);
}

int fputs(const char* s, int fd) {
    if (!s) return EOF;
    size_t len = strlen(s);
    return write(fd, s, len);
}
