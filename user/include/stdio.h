#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)

/* Buffered I/O functions */
int  getchar(void);
int  putchar(int c);
int  puts(const char* s);
int  printf(const char* format, ...);
int  vprintf(const char* format, va_list args);
void flush_stdout(void);

int scanf(const char* format, ...);
int vscanf(const char* format, va_list args);

/* File descriptor I/O */
int fputc(int c, int fd);
int fputs(const char* s, int fd);
int fgetc(int fd);
int fgets(char* buf, size_t size, int fd);

#endif // STDIO_H
