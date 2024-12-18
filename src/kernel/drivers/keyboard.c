#include "kernel/drivers/keyboard.h"  
#include "kernel/terminal.h"
#include "kernel/drivers/port_io.h"
#include "kernel/interrupts/isr.h"

// Map of scancodes to ASCII characters
static const char keyboard_map[128] = {
0, 0 /* Escape */, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0 /* Backspace */,
0 /* Tab */, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n' /* Enter */,
0 /* Left Ctrl */, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
0 /* Left Shift */, '\\' /* TODO: CHECK*/, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0 /* Right Shift */,
'*', 0 /* Alt */, ' ', 0 /* Caps Lock */,
0 /* f1 */, 0 /* f2 */, 0 /* f3 */, 0 /* f4 */, 0 /* f5 */, 0 /* f6 */, 0 /* f7 */, 0 /* f8 */, 0 /* f9 */, 0 /* f10 */,
0 /* Num Lock */, 0 /* Scroll Lock */,0 /* Home */, 0 /* Up Arrow */, 0 /* Page Up */, '-', 0 /* Left Arrow */, 0, 0 /* Right Arrow */, '+',
0 /* End */, 0 /* Down Arrow */, 0 /* Page Down */, 0 /* Insert */, 0 /* Delete */,
};
static uint64_t keys[2] = {0};

// Convert a scancode to an ASCII character
char scancode_to_ascii(uint8_t scancode) {
    if (scancode < 0x80) {
        return keyboard_map[scancode];
    }
    return 0;
}

// Handle keypress (print the corresponding ASCII character)
void handle_keypress(uint8_t scancode) {
    keys[(scancode >> 6) & 0x01] ^= 1UL << (scancode & 0x3f);
    if (scancode >= 0x80)
        return;

    char c = scancode_to_ascii(scancode);
    terminal_input(scancode, c);
}