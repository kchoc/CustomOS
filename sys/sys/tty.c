#include "tty.h"

#include <kern/errno.h>
#include <kern/spinlock.h>

file_ops_t tty_file_ops = {
    .read  = (int (*)(file_t*, char*, size_t, size_t))tty_read,
    .write = (int (*)(file_t*, const char*, size_t, size_t))tty_write,
};

tty_t ttys[4];

int tty_init(void)
{
    for (int i = 0; i < 4; i++) {
        ttys[i].termios.c_iflag = 0;
        ttys[i].termios.c_oflag = 0;
        ttys[i].termios.c_cflag = 0;
        ttys[i].termios.c_lflag = 0;
        ttys[i].inlen           = 0;
        ttys[i].outlen          = 0;
        ttys[i].lock            = 0;
        ttys[i].console         = NULL; // Will be set when the console is initialized
    }
    return 0; // Success
}

int tty_write(file_t* file, const char* data, size_t len, size_t offset)
{
    tty_t* tty = (tty_t*)file->private;

    if (!tty || !tty->console)
        return -EINVAL; // Invalid TTY or console

    WITH_SPINLOCK(tty->lock)

    for (size_t i = 0; i < len; i++)
        tty->console->putc(tty->console, data[i]);

    END_WITH_SPINLOCK

    return len; // Return the number of bytes written
}

int tty_read(tty_t* tty, char* buffer, size_t len)
{
    if (!tty)
        return -EINVAL; // Invalid TTY

    size_t to_read;
    WITH_SPINLOCK(tty->lock)

    to_read = (len < tty->inlen) ? len : tty->inlen;
    for (size_t i = 0; i < to_read; i++)
        buffer[i] = tty->inbuf[i];

    // Shift remaining input to the front of the buffer
    for (size_t i = to_read; i < tty->inlen; i++)
        tty->inbuf[i - to_read] = tty->inbuf[i];
    tty->inlen -= to_read;

    END_WITH_SPINLOCK

    return to_read; // Return the number of bytes read
}
