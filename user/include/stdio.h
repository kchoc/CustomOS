#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

/* Buffered I/O functions */
int getchar(void);
int putchar(int c);
int puts(const char* s);
int printf(const char* format, ...);
int vprintf(const char* format, va_list args);
void flush_stdout(void);

/* File descriptor I/O */
int fputc(int c, int fd);
int fputs(const char* s, int fd);

#endif // STDIO_H
