#ifndef SYS_TERMIOS_H
#define SYS_TERMIOS_H

#include <inttypes.h>

/***********************
 * Terminal I/O Settings
 ***********************/
typedef enum {
    IGNBRK = 0x0001, // Ignore break condition
    BRKINT = 0x0002, // Signal interrupt on break
    IGNPAR = 0x0004, // Ignore characters with parity errors
    PARMRK = 0x0008, // Mark parity errors
    INPCK  = 0x0010, // Enable input parity checking
    ISTRIP = 0x0020, // Strip character
    INLCR  = 0x0040, // Map NL to CR on input
    IGNCR  = 0x0080, // Ignore carriage return on input
    ICRNL  = 0x0100, // Map CR to NL on input
    IXON   = 0x0200, // Enable start/stop output control
} c_iflag_t;

typedef enum {
    OPOST  = 0x0001, // Post-process output
    ONLCR  = 0x0002, // Map NL to CR-NL on output
    OCRNL  = 0x0004, // Map CR to NL on output
    ONOCR  = 0x0008, // No CR output at column 0
    ONLRET = 0x0010, // NL performs a carriage return
} c_oflag_t;

typedef enum {
    CREAD  = 0x0001, // Enable receiver
    CSIZE  = 0x0006, // Character size mask
    CS5    = 0x0000, // 5 bits per byte
    CS6    = 0x0002, // 6 bits per byte
    CS7    = 0x0004, // 7 bits per byte
    CS8    = 0x0006, // 8 bits per byte
    PARENB = 0x0008, // Enable parity generation on output and parity checking for input
    PARODD = 0x0010, // Use odd parity instead of even
} c_cflag_t;

typedef enum {
    ISIG   = 0x0001, // Enable signals
    ICANON = 0x0002, // Canonical input (line editing)
    ECHO   = 0x0004, // Enable echo
    ECHOE  = 0x0008, // Echo erase character as BS-SP-BS
    ECHOK  = 0x0010, // Echo KILL character by erasing current line
    IEXTEN = 0x0020, // Enable extended input processing
} c_lflag_t;

typedef enum {
    VEOF    = 0,  // End-of-file character
    VEOL    = 1,  // End-of-line character
    VERASE  = 2,  // Erase character
    VINTR   = 3,  // Interrupt character
    VIKILL  = 4,  // Kill-line character
    VMIN    = 5,  // Minimum number of characters for non-canonical read
    VQUIT   = 6,  // Quit character
    VSTART  = 7,  // Start character
    VSTOP   = 8,  // Stop character
    VSUSP   = 9,  // Suspend character
    VTIME   = 10, // Timeout in deciseconds for non-canonical read
    VWERASE = 11, // Word erase character
    NCCS    = 32  // Size of c_cc array
} cc_index_t;

typedef struct termios {
    c_iflag_t c_iflag;    // Input modes
    c_oflag_t c_oflag;    // Output modes
    c_cflag_t c_cflag;    // Control modes
    c_lflag_t c_lflag;    // Local modes
    uint8_t   c_cc[NCCS]; // Control characters
} termios_t;

#endif // SYS_TERMIOS_H
