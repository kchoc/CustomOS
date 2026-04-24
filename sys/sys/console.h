#ifndef SYS_CONSOLE_H
#define SYS_CONSOLE_H

#include "device.h"

typedef struct console {
    void (*putc)(struct console* console, char c); // Function to output a character to the console
    void (*clear)(struct console* console);        // Function to clear the console screen

    device_t* dev; // Console device (e.g., VGA)
} console_t;

#define DECLARE_CONSOLE_TYPE(name)                                                                 \
    extern console_t name##_console;                                                               \
    void             name##_console_putc(console_t* console, char c);                              \
    void             name##_console_clear(console_t* console);

#endif // SYS_CONSOLE_H
