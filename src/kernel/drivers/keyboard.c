#include "kernel/drivers/keyboard.h"  
#include "kernel/terminal.h"
#include "kernel/drivers/port_io.h"
#include "kernel/interrupts/isr.h"

// Map of scancodes to ASCII characters
static const char keyboard_map[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', 0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Convert a scancode to an ASCII character
char scancode_to_ascii(uint8_t scancode) {
    if (scancode < 0x80) {
        return keyboard_map[scancode];
    }
    return 0;
}

// Handle keypress (print the corresponding ASCII character)
void handle_keypress(uint8_t scancode) {
    char c = scancode_to_ascii(scancode);
    if (c != 0) {
        terminal_putchar(c);
    }
}