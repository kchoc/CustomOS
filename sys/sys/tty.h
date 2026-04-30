#ifndef DEV_TTY_H
#define DEV_TTY_H

#include "termios.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct tty tty_t;

/* -----------------------------------------------------------------------
   Ring buffer
   ----------------------------------------------------------------------- */
#define TTY_BUF_SIZE 4096   /* must be a power of two */
#define TTY_BUF_MASK (TTY_BUF_SIZE - 1)

typedef struct tty_ring {
    char     buf[TTY_BUF_SIZE];
    size_t   head;   /* producer writes here */
    size_t   tail;   /* consumer reads here  */
} tty_ring_t;

static inline size_t tty_ring_len(const tty_ring_t* r)
{
    return (r->head - r->tail) & TTY_BUF_MASK;
}

static inline bool tty_ring_empty(const tty_ring_t* r)
{
    return r->head == r->tail;
}

static inline bool tty_ring_full(const tty_ring_t* r)
{
    return ((r->head + 1) & TTY_BUF_MASK) == r->tail;
}

/* -----------------------------------------------------------------------
 * TTY Window Size
 * ----------------------------------------------------------------------- */
typedef struct winsize {
    uint16_t ws_row;    /* rows, in characters */
    uint16_t ws_col;    /* columns, in characters */
} winsize_t;

/* -----------------------------------------------------------------------
   TTY IOCTL definitions (for ioctl() calls from user space)
   ----------------------------------------------------------------------- */
typedef enum {
    TTY_IOCTL_IFLUSH        = 0,    /* flush input buffer */
    TTY_IOCTL_OFLOW         = 1,    /* set output flow control (boolean arg) */
    TTY_IOCTL_OFLUSH        = 2,    /* flush output buffer */ 
    TTY_IOCTL_GETS          = 5401, /* get termios settings (arg is pointer to struct termios) */
    TTY_IOCTL_SETS          = 5402, /* set termios settings (arg is pointer to struct termios) */
    TTY_IOCTL_SETSW         = 5403, /* set termios settings and wait for output to drain */ 
    TTY_IOCTL_SETSF         = 5404, /* set termios settings and flush input buffer */ 
    TTY_IOCTL_GETWINSZ      = 5405, /* get window size (arg is pointer to struct winsize) */ 
    TTY_IOCTL_SETWINSZ      = 5406, /* set window size (arg is pointer to struct winsize) */ 
    TTY_IOCTL_GETPGRP       = 5407, /* get foreground process group ID (arg is pointer to pid_t) */
    TTY_IOCTL_SETPGRP       = 5408, /* set foreground process group ID (arg is pointer to pid_t) */ 
    TTY_IOCTL_SETCONTROLTTY = 5409, /* set this TTY as the controlling terminal for the current process */ 
    TTY_IOCTL_FLUSH         = 5410, /* flush input and/or output buffers (arg is int mask: 1=input, 2=output) */ 
    TTY_IOCTL_COUTQ         = 5411, /* get number of bytes in output queue (arg is pointer to int) */
    TTY_IOCTL_READQ         = 5412, /* get number of bytes in input queue (arg is pointer to int) */
    TTY_IOCTL_SETOUTPUTDEV = 5413, /* set output callback device (arg is pointer to device_t) */
} tty_ioctl_cmd_t;

typedef enum {
    TTY_SIG_INT  = 1,    /* SIGINT (e.g. Ctrl-C) */
    TTY_SIG_QUIT = 2,    /* SIGQUIT (e.g. Ctrl-\) */
    TTY_SIG_STOP = 3,    /* SIGSTOP (e.g. Ctrl-S) */
    TTY_SIG_SUSP = 4,    /* SIGTSTP (e.g. Ctrl-Z) */ 
    TTY_SIG_WINCH = 5,   /* SIGWINCH (window size change) */
} tty_signal_t;

/* -----------------------------------------------------------------------
   TTY ops
   ----------------------------------------------------------------------- */
typedef struct tty_ops {
    const char* name;   /* for debugging */ 
    void (*rx_char)(tty_t* tty, char c, uint8_t flags);  /* called by keyboard IRQ handler with input character */
    ssize_t (*tx_chars)(tty_t* tty, const char* buf, size_t count); /* called by tty_write to send output to hardware */
    void (*set_termios)(tty_t* tty, const termios_t* t); /* called by ioctl to set termios settings in hardware driver */ 
    void (*rx_flush)(tty_t* tty); /* called by tty_input to flush completed lines to in_ring */ 
    void (*tx_flush)(tty_t* tty); /* called by tty_flush to flush output ring to hardware */
} tty_ops_t;

typedef enum {
    TTY_RX_FLAG_NONE = 0,
    TTY_RX_FLAG_BREAK = 1,   /* break condition (line held low) */
    TTY_RX_FLAG_ERROR = 2,   /* framing/parity error on received character */ 
} tty_rx_flag_t;

/* -----------------------------------------------------------------------
   TTY struct
   ----------------------------------------------------------------------- */
typedef struct tty {
    tty_ops_t* tty_ops;
    int pgrp; /* foreground process group ID, for signal delivery */

    /* --- Queues --- */
    tty_ring_t in_ring;
    tty_ring_t out_ring;
    tty_ring_t raw_ring;

    size_t in_line_len;   /* length of current line being input (for canonical mode) */

    /* --- State --- */
    termios_t termios;
    winsize_t winsize;

    /* --- Callbacks for line discipline --- */
    void (*output)(tty_t* tty, const char* buf, size_t count); /* helper to write output through line discipline to out_ring */ 
    void *output_data; /* opaque pointer passed to output() */

    /* --- Signals --- */
    void (*raise_signal)(tty_t* tty, tty_signal_t sig); /* helper to send signal to foreground process group */
} tty_t;

/* -----------------------------------------------------------------------
   API
   ----------------------------------------------------------------------- */

DECLARE_DEVICE_TYPE(tty);
void tty_input(tty_t* tty, char c, uint8_t flags); /* called by keyboard IRQ handler with input character */ 
int tty_init();

#endif // DEV_TTY_H
