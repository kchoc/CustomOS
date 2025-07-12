#include "kernel/drivers/keyboard.h"  
#include "kernel/terminal.h"
#include "kernel/drivers/port_io.h"
#include "kernel/interrupts/isr.h"
#include <stdbool.h>

// Base keyboard map
static const char keyboard_map[128] = {
    0, 0 /* Esc */, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0 /* Ctrl */, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0 /* Left Shift */, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0 /* Right Shift */,
    '*', 0 /* Alt */, ' ', 0 /* Caps Lock */,
    0,0,0,0,0,0,0,0,0,0, // F1–F10
    0,0,0,0,0,'-',0,0,0,'+',
    0,0,0,0,0
};

// Shifted version of the keyboard map
static const char keyboard_shift_map[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0 /* Ctrl */, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,'-',0,0,0,'+',
    0,0,0,0,0
};

// Key state bitmask
static bool shift = false;
static bool ctrl = false;

static uint64_t keys[2] = {0};

// Convert scancode to ASCII, considering modifiers
char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 0x80) return 0;

    char base = (shift ? keyboard_shift_map : keyboard_map)[scancode];

    // Optionally, handle Ctrl combinations
    if (ctrl && base >= 'a' && base <= 'z') {
        return base - 'a' + 1; // Ctrl+A = 0x01, Ctrl+B = 0x02, etc.
    }

    return base;
}

// Handle key press/release
void handle_keypress(uint8_t scancode) {
    bool is_release = scancode & 0x80;
    uint8_t clean = scancode & 0x7F;

    keys[(scancode >> 6) & 0x01] ^= 1UL << (scancode & 0x3F);

    // Track modifier state
    switch (clean) {
        case 0x2A: // Left Shift
        case 0x36: // Right Shift
            shift = !is_release;
            return;
        case 0x1D: // Ctrl
            ctrl = !is_release;
            return;
    }

    if (is_release) return; // Ignore key releases for characters

    char c = scancode_to_ascii(scancode);
    terminal_input(scancode, c);
}
