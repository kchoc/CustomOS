#include "tty.h"

#include <dev/vga/vga.h>
#include <dev/input/keyboard.h>

#include <sys/device.h>

#include <kern/errno.h>

#include <string.h>

/* -----------------------------------------------------------------------
   Ring buffer helpers (internal)
   ----------------------------------------------------------------------- */

static int ring_push(tty_ring_t* r, char c)
{
    if (tty_ring_full(r))
        return -1; // buffer full
    r->buf[r->head] = c;
    r->head = (r->head + 1) & TTY_BUF_MASK;
    return 0;
}

static char ring_pop(tty_ring_t* r)
{
    if (tty_ring_empty(r))
        return -1; // buffer empty
    char c = r->buf[r->tail & TTY_BUF_MASK];
    r->tail++;
    return c;
}

static char ring_peek(const tty_ring_t* r, size_t index)
{
    return r->buf[(r->tail + index) & TTY_BUF_MASK];
}

static int ring_unpush(tty_ring_t* r)
{
    if (r->head == r->tail)
        return -1; // buffer empty
    r->head = (r->head - 1) & TTY_BUF_MASK;
    return 0;
}

/* =========================================================================
 * VGA Output backend - this might be better implemented as part of VGA driver
 * ========================================================================= */ 
static void tty_vga_output(tty_t* tty, const char* buf, size_t count)
{
    device_t* dev = (device_t*)tty->output_data;
    if (!dev) return;
    dev->ops->write(dev, 0, (uint32_t)count, (const uint8_t*)buf);
}

/* =========================================================================
 * Line discipline helpers (internal)
 * ========================================================================= */ 
static void tty_echo(tty_t* tty, char c)
{
    if (!(tty->termios.c_lflag & ECHO)) return;

    if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') {
        // Control character: echo as ^X
        char echo_buf[2] = {'^', (char)(c + 0x40)};
        tty->output(tty, echo_buf, 2);
    } else {
        // Regular character
        tty->output(tty, &c, 1);
    }
}

static void tty_echo_erase(tty_t* tty, uint8_t erased)
{
    if (!(tty->termios.c_lflag & ECHO)) return;

    int cols = (erased < 0x20 && erased != '\n' && erased != '\t') ? 2 : 1;
    uint8_t backspaces[] = { '\b', ' ', '\b', '\b', ' ', '\b' };
    tty->output(tty, backspaces, cols * 3);
}

static void tty_rx_char(tty_t* tty, char c, uint8_t flags)
{
    termios_t* t = &tty->termios;

    if (t->c_iflag & ISTRIP) c &= 0x7F; // Strip to 7 bits
    if (t->c_iflag & INLCR)  c = (c == '\n') ? '\r' : c; // Map NL to CR
    if (t->c_iflag & IGNCR)  if (c == '\r') return; // Ignore CR
    if (t->c_iflag & ICRNL)  c = (c == '\r') ? '\n' : c; // Map CR to NL
    if (t->c_iflag & IXON)   {
        if (c == t->c_cc[VSTART]) return; // Start output (ignored for now)
        if (c == t->c_cc[VSTOP]) return;  // Stop output (ignored for now)
    }
    
    if (t->c_lflag & ISIG) {
        if (c == t->c_cc[VINTR]) {
            if (tty->raise_signal) tty->raise_signal(tty, TTY_SIG_INT);
            tty_echo(tty, c); 
            // Flush the input buffer
            memset(&tty->in_ring, 0, sizeof(tty_ring_t));
            tty->in_line_len = 0;
            return;
        }
        if (c == t->c_cc[VQUIT]) {
            if (tty->raise_signal) tty->raise_signal(tty, TTY_SIG_QUIT);
            tty_echo(tty, c);

            memset(&tty->in_ring, 0, sizeof(tty_ring_t));
            tty->in_line_len = 0;
            return;
        }
        if (c == t->c_cc[VSUSP]) {
            if (tty->raise_signal) tty->raise_signal(tty, TTY_SIG_SUSP);
            tty_echo(tty, c);
            return;
        }
    }

    if (t->c_lflag & ICANON) {
        if (c == t->c_cc[VERASE] || c == '\b') {
            if (tty->in_line_len > 0) {
                uint8_t erased = ring_peek(&tty->in_ring, tty->in_line_len - 1);
                ring_unpush(&tty->in_ring);
                tty->in_line_len--;
                if (tty->termios.c_lflag & ECHOE)
                    tty_echo_erase(tty, erased);
            }
            return;
        }
        if (c == t->c_cc[VIKILL]) {
            while (tty->in_line_len > 0) {
                uint8_t erased = ring_peek(&tty->in_ring, tty->in_line_len - 1);
                ring_unpush(&tty->in_ring);
                tty->in_line_len--;
                if (tty->termios.c_lflag & ECHOK)
                    tty_echo_erase(tty, erased); 
            }
            if (tty->termios.c_lflag & ECHOK) {
                char nl = '\n';
                tty->output(tty, &nl, 1);
            }
            return;
        }
        if ((t->c_lflag & IEXTEN) && c == t->c_cc[VWERASE]) {
            int phase = 0;
            while (tty->in_line_len > 0) {
                uint8_t peeked = ring_peek(&tty->in_ring, tty->in_line_len - 1);
                if (phase == 0 && peeked != ' ') phase = 1;
                if (phase == 1 && peeked == ' ') break;

                ring_unpush(&tty->in_ring);
                tty->in_line_len--;
                if (tty->termios.c_lflag & ECHOE)
                    tty_echo_erase(tty, peeked);
            }
             return;
        }
        if (c == t->c_cc[VEOF]) {
            ring_push(&tty->in_ring, 0xFF); // EOF marker
            tty->in_line_len = 0;
            return;
        }
        if (!tty_ring_full(&tty->in_ring)) {
            ring_push(&tty->in_ring, c);
            tty->in_line_len++;
            tty_echo(tty, c);
        }

        if (c == '\n' || c == t->c_cc[VEOL]) {
            tty->in_line_len = 0; // reset line length for next line
        }
        return;
    }

    if (!tty_ring_full(&tty->in_ring)) {
        ring_push(&tty->in_ring, c);
        tty_echo(tty, c);
    }
}

static ssize_t tty_tx_chars(tty_t* tty, const char* buf, size_t count)
{
    if (!tty->output) return count; // no output callback, just discard silently

    if (!(tty->termios.c_oflag & OPOST)) {
        // No post-processing, write raw bytes
        tty->output(tty, buf, count);
        return (ssize_t)count;
    }

    // Post-process output characters
    size_t i = 0;
    size_t start = 0;

    while (i < count) {
        char c = buf[i];
        if (c == '\n' && (tty->termios.c_oflag & ONLCR)) {
            // Map NL to CR-NL
            if (i > start)
                tty->output(tty, &buf[start], i - start);
            char crnl[] = {'\r', '\n'};
            tty->output(tty, crnl, 2);
            start = i + 1;
        } else if (c == '\r' && (tty->termios.c_oflag & OCRNL)) {
            // Map CR to NL
            if (i > start)
                tty->output(tty, &buf[start], i - start);
            char nl = '\n';
            tty->output(tty, &nl, 1);
            start = i + 1;
        } else if (c == '\r' && (tty->termios.c_oflag & ONOCR) && i == 0) {
            // No CR output at column 0
            start = i + 1;
        } else {
            i++;
        }
    }

    if (i > start)
        tty->output(tty, &buf[start], i - start);

    return (ssize_t)count;
}

static void tty_set_termios(tty_t* tty, const termios_t* t)
{
    if (!tty || !t) return;
    tty->termios = *t;
}

static void tty_rx_flush(tty_t* tty)
{
    memset(&tty->raw_ring, 0, sizeof(tty_ring_t));
    tty->in_line_len = 0;
}

static void tty_tx_flush(tty_t* tty)
{
    memset(&tty->out_ring, 0, sizeof(tty_ring_t));
}

tty_ops_t default_tty_ops = {
    .name = "Default TTY Line Discipline",
    .rx_char = tty_rx_char,
    .tx_chars = tty_tx_chars,
    .set_termios = tty_set_termios,
    .rx_flush = tty_rx_flush,
    .tx_flush = tty_tx_flush
};

/* =========================================================================
 * TTY driver implementation
 * ========================================================================= */

int tty_probe(device_t* dev)
{
    if (!dev) return -ENODEV;
    tty_t* tty = kmalloc(sizeof(tty_t));
    if (!tty) return -ENOMEM;
    dev->ops_data = tty;
    return 0;
}

driver_t tty_driver = {
    .name        = "tty",
    .vendor_id   = 0,
    .device_id   = 0,
    .device_type = DEV_TYPE_CHAR,
    .probe       = tty_probe
};

/* =========================================================================
 * TTY input handling (called by keyboard IRQ handler)
 * ========================================================================= */
void tty_input(tty_t* tty, char c, uint8_t flags)
{
    if (!tty || !tty->tty_ops || !tty->tty_ops->rx_char) return;
    // SPINLOCK REQUIRED
    tty->tty_ops->rx_char(tty, c, flags);
}

/* =========================================================================
 * tty device ops
 * ========================================================================= */ 

int tty_open(device_t* dev)
{
    if (!dev) return -ENODEV;
    tty_t* tty = (tty_t*)dev->ops_data;
    if (!tty) return -ENODEV;

    // Initialise TTY state
    memset(&tty->in_ring, 0, sizeof(tty_ring_t));
    memset(&tty->out_ring, 0, sizeof(tty_ring_t));
    memset(&tty->raw_ring, 0, sizeof(tty_ring_t));
    tty->in_line_len = 0;
    tty->tty_ops = &default_tty_ops;
    memset(&tty->termios, 0, sizeof(termios_t));
    memset(&tty->winsize, 0, sizeof(winsize_t));

    // Set default termios settings (e.g. 9600 baud, 8N1)
    tty->termios.c_iflag = ICRNL | IXON;
    tty->termios.c_oflag = OPOST | ONLCR;
    tty->termios.c_cflag = CREAD | CS8;
    tty->termios.c_lflag = ICANON | ECHO | ECHOE | ECHOK;
    tty->termios.c_cc[VEOF] = 4;   // Ctrl-D
    tty->termios.c_cc[VEOL] = 0;   // disabled
    tty->termios.c_cc[VERASE] = 127; // DEL
    tty->termios.c_cc[VINTR] = 3;   // Ctrl-C
    tty->termios.c_cc[VIKILL] = 21; // Ctrl-U
    tty->termios.c_cc[VMIN] = 1;    // read() blocks until at least 1 char available
    tty->termios.c_cc[VQUIT] = 28;  // Ctrl-\
    tty->termios.c_cc[VSTART] = 17; // Ctrl-Q
    tty->termios.c_cc[VSTOP] = 19;  // Ctrl-S
    tty->termios.c_cc[VSUSP] = 26;  // Ctrl-Z
    tty->termios.c_cc[VTIME] = 0;   // no timeout

    keyboard_set_active_tty(tty); // set this TTY as active for keyboard input

    return 0;
}

int tty_close(device_t* dev)
{
    if (!dev) return -ENODEV;
    
    tty_t* tty = (tty_t*)dev->ops_data;
    if (!tty) return -ENODEV;

    if (tty->tty_ops && tty->tty_ops->tx_flush)
        tty->tty_ops->tx_flush(tty); // flush output before closing

    kfree(tty);
    dev->ops_data = NULL;
    return 0;
}

int tty_read(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buf)
{
    if (!dev) return -ENODEV;
    if (!buf) return -EFAULT;
    if (size == 0) return 0;

    tty_t* tty = (tty_t*)dev->ops_data;
    if (!tty) return -EBADF;

    uint8_t* dst = buf;
    size_t bytes_read = 0;

    if (tty->termios.c_lflag & ICANON) {
        for (;;) {
          // SPINLOCK REQUIRED
          size_t avail = tty_ring_len(&tty->in_ring);
          int complete = 0;
          for (size_t i = 0; i < avail; i++) {
              uint8_t c = ring_peek(&tty->in_ring, i);
              if (c == 0xFF || c == '\n') {
                  complete = 1;
                  break;
              }
          }

          if (complete) break; // at least one complete line available

          yield(); // wait for more input
        }

        // Read up to the first complete line
        // SPINLOCK REQUIRED
        while (bytes_read < size && !tty_ring_empty(&tty->in_ring)) {
            uint8_t c = ring_pop(&tty->in_ring);
            if (c == 0xFF) break; // EOF marker
            dst[bytes_read++] = c;
            if (c == '\n') break; // end of line
        }
    } else {
        // Raw mode: read whatever is available, up to 'size'
        // Wait for VMIN characters to be available
        uint8_t vmin = tty->termios.c_cc[VMIN];
        if (vmin == 0) vmin = 1;
        while (tty_ring_len(&tty->in_ring) < vmin) {
            yield(); // wait for more input
        }

        // SPINLOCK REQUIRED
        while (bytes_read < size && !tty_ring_empty(&tty->in_ring)) {
            dst[bytes_read++] = ring_pop(&tty->in_ring);
        }
    }
    
    return (ssize_t)bytes_read;
}

int tty_write(device_t* dev, uint64_t offset, uint32_t count, const uint8_t* buf)
{
    if (!dev) return -ENODEV;
    if (!buf) return -EFAULT;
    if (count == 0) return 0;

    tty_t* tty = (tty_t*)dev->ops_data;
    if (!tty) return -ENODEV;

    if (!tty->tty_ops || !tty->tty_ops->tx_chars) return -EBADF;

    return tty->tty_ops->tx_chars(tty, (const char*)buf, count);
}

int tty_ioctl(device_t* dev, int cmd, void* arg)
{
    if (!dev) return -ENODEV;
    tty_t* tty = (tty_t*)dev->ops_data;
    if (!tty) return -ENODEV;

    // For simplicity, we only implement a few ioctl commands
    switch (cmd) {
    case TTY_IOCTL_COUTQ: {
        int* out = (int*)arg;
        if (!out) return -EFAULT;
        *out = (int)(tty->out_ring.head - tty->out_ring.tail);
        return 0;
    }
    case TTY_IOCTL_READQ: {
        int* out = (int*)arg;
        if (!out) return -EFAULT;
        *out = (int)(tty->in_ring.head - tty->in_ring.tail);
        return 0;
    }
    case TTY_IOCTL_SETOUTPUTDEV: {
        device_t* out_dev = (device_t*)arg;
        if (!out_dev) return -EFAULT;
        tty->output_data = out_dev;
        tty->output = tty_vga_output; // set output callback to write to VGA
        return 0;
    }
    default:
        return -EINVAL; // unsupported command
    }
}

device_ops_t tty_ops = {
    .open  = tty_open,
    .read  = tty_read,
    .write = tty_write,
    .ioctl = tty_ioctl,
    .close = tty_close
};

/* =========================================================================
 * TTY initialization
 * ========================================================================= */ 
int tty_init(void)
{
    device_t* dev;
    int res = device_misc_create(&tty_driver, &tty_ops, &dev);
    if (res) return res;

    res = vfs_register_device(dev);
    if (res) return res;

    return 0;
}

