#include "vga.h"
#include <dev/port/port_io.h>
#include <kern/terminal.h>
#include <string.h>

// VGA Register ports
#define VGA_MISC_WRITE      0x3C2
#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5
#define VGA_CRTC_INDEX      0x3D4
#define VGA_CRTC_DATA       0x3D5
#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA         0x3CF
#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ         0x3C1
#define VGA_INSTAT_READ     0x3DA

static void write_regs(const uint8_t *regs, uint16_t count, uint16_t port_index, uint16_t port_data) {
    for (uint16_t i = 0; i < count; i++) {
        outb(port_index, i);
        outb(port_data, regs[i]);
    }
}

void vga_set_mode_13h(void) {
    // Mode 13h register values
    static const uint8_t g_320x200x256[] = {
        /* MISC */
        0x63,
        /* SEQ */
        0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF,
        /* GC */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
        0xFF,
        /* AC */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41, 0x00, 0x0F, 0x00, 0x00
    };

    // Write MISC register
    outb(VGA_MISC_WRITE, g_320x200x256[0]);

    // Write SEQ registers
    write_regs(g_320x200x256 + 1, 5, VGA_SEQ_INDEX, VGA_SEQ_DATA);

    // Unlock CRTC registers
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);

    // Write CRTC registers
    write_regs(g_320x200x256 + 6, 25, VGA_CRTC_INDEX, VGA_CRTC_DATA);

    // Write GC registers
    write_regs(g_320x200x256 + 31, 9, VGA_GC_INDEX, VGA_GC_DATA);

    // Write AC registers
    inb(VGA_INSTAT_READ);  // Reset flip-flop
    for (uint8_t i = 0; i < 21; i++) {
        inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, g_320x200x256[40 + i]);
    }
    outb(VGA_AC_INDEX, 0x20);  // Enable video

    // Clear screen (use kernel-mapped address)
    uint8_t* fb = (uint8_t*)0xC0400000;
    memset(fb, 0, 320 * 200);
}

void vga_set_mode_text(void) {
    // Mode 3 (80x25 text) register values
    static const uint8_t g_80x25_text[] = {
        /* MISC */
        0x67,
        /* SEQ */
        0x03, 0x00, 0x03, 0x00, 0x02,
        /* CRTC */
        0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
        0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
        0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
        0xFF,
        /* GC */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
        0xFF,
        /* AC */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x0C, 0x00, 0x0F, 0x08, 0x00
    };

    // Write MISC register
    outb(VGA_MISC_WRITE, g_80x25_text[0]);

    // Write SEQ registers
    write_regs(g_80x25_text + 1, 5, VGA_SEQ_INDEX, VGA_SEQ_DATA);

    // Unlock CRTC registers
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);

    // Write CRTC registers
    write_regs(g_80x25_text + 6, 25, VGA_CRTC_INDEX, VGA_CRTC_DATA);

    // Write GC registers
    write_regs(g_80x25_text + 31, 9, VGA_GC_INDEX, VGA_GC_DATA);

    // Write AC registers
    inb(VGA_INSTAT_READ);  // Reset flip-flop
    for (uint8_t i = 0; i < 21; i++) {
        inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, g_80x25_text[40 + i]);
    }
    outb(VGA_AC_INDEX, 0x20);  // Enable video
    
    // Reinitialize terminal to restore text output
    terminal_init();
}
