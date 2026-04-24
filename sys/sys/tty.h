#ifndef SYS_TTY_H
#define SYS_TTY_H

#include <fs/file.h>

#include "console.h"
#include "termios.h"

#include <kern/spinlock.h>

#define TTY_BUF 1024

typedef struct tty_ops tty_ops_t;

typedef struct tty {
    termios_t termios; // Terminal I/O settings

    char   inbuf[TTY_BUF];
    size_t inlen;

    char   outbuf[TTY_BUF];
    size_t outlen;

    spinlock_t lock; // Spinlock for synchronizing access
    console_t* console;
} tty_t;

int tty_init(void);
int tty_write(file_t* file, const char* data, size_t len, size_t offset);
int tty_read(file_t* file, char* buffer, size_t len, size_t offset);

#endif // SYS_TTY_H
