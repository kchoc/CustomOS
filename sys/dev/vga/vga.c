#include "vga.h"

#include <dev/port/port_io.h>
#include <sys/tty.h>
#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

DECLARE_DRIVER(vga, root);

/* =========================================================================
 * Hardware register tables for standard modes
 * ========================================================================= */

/* Register dump layout: { misc, seq[5], crtc[25], gc[9], ac[21] } */
typedef struct {
    uint8_t misc;
    uint8_t seq[5];
    uint8_t crtc[25];
    uint8_t gc[9];
    uint8_t ac[21];
} vga_regs_t;

/* --- Mode 03h: 80×25 colour text ---------------------------------------- */
static const vga_regs_t g_regs_text80x25 = {
    /* Miscellaneous Output */
    .misc = 0x67,

    /* Sequencer: Reset, Clocking, Plane Write Enable, Char Map, Mem Mode */
    .seq = {0x00, 0x00, 0x03, 0x00, 0x02},

    /* CRTC (0x00–0x18) */
    .crtc = {0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E, 0x00,
             0x00, 0x00, 0x50, 0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF},

    /* Graphics Controller (0x00–0x08) */
    .gc = {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF},

    /* Attribute Controller (0x00–0x14) */
    .ac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A,
           0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00}};

/* --- Mode 13h: 320×200, 256 colours -------------------------------------- */
static const vga_regs_t g_regs_mode13h = {
    .misc = 0x63,
    .seq  = {0x00, 0x01, 0x0F, 0x00, 0x0E},
    .crtc = {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF},
    .gc   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF},
    .ac   = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
             0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00}};

/* =========================================================================
 * Internal helpers: register programming
 * ========================================================================= */

static void write_regs(const vga_regs_t* r)
{
    /* 1. Miscellaneous Output */
    outb(VGA_MISC_WRITE, r->misc);

    /* 2. Sequencer — release reset last */
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x00); /* synchronous reset */
    for (int i = 1; i < 5; i++) {
        outb(VGA_SEQ_INDEX, (uint8_t)i);
        outb(VGA_SEQ_DATA, r->seq[i]);
    }
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x03); /* normal operation  */

    /* 3. Unlock CRTC registers 0–7 (clear protect bit in reg 0x11) */
    outb(VGA_CRTC_INDEX, VGA_CRTC_VRETRACE_END);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);

    for (int i = 0; i < 25; i++) {
        outb(VGA_CRTC_INDEX, (uint8_t)i);
        outb(VGA_CRTC_DATA, r->crtc[i]);
    }

    /* 4. Graphics Controller */
    for (int i = 0; i < 9; i++) {
        outb(VGA_GC_INDEX, (uint8_t)i);
        outb(VGA_GC_DATA, r->gc[i]);
    }

    /* 5. Attribute Controller — reset flip-flop, then write */
    inb(VGA_INSTAT1_READ); /* reset AC flip-flop */
    for (int i = 0; i < 21; i++) {
        outb(VGA_AC_INDEX, (uint8_t)i);
        outb(VGA_AC_WRITE, r->ac[i]);
    }
    outb(VGA_AC_INDEX, 0x20); /* enable display     */
}

/* =========================================================================
 * Internal helpers: cursor
 * ========================================================================= */

static void hw_set_cursor(uint16_t pos)
{
    outb(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_HI);
    outb(VGA_CRTC_DATA, (uint8_t)(pos >> 8));
    outb(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_LO);
    outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
}

static void hw_cursor_visible(int visible)
{
    outb(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_HI);
    uint8_t v = inb(VGA_CRTC_DATA);
    if (visible)
        v &= ~0x20; /* clear disable bit */
    else
        v |= 0x20; /* set disable bit   */
    outb(VGA_CRTC_DATA, v);
}

/* =========================================================================
 * Internal helpers: text output
 * ========================================================================= */

static void text_write_cell(vga_device_t* dev, uint16_t x, uint16_t y, char c, uint8_t attr)
{
    uint16_t idx       = (uint16_t)(y * VGA_TEXT_COLS + x);
    dev->text_buf[idx] = (uint16_t)((attr << 8) | (uint8_t)c);
}

static void text_advance_cursor(vga_device_t* dev)
{
    dev->cursor_x++;
    if (dev->cursor_x >= VGA_TEXT_COLS) {
        dev->cursor_x = 0;
        dev->cursor_y++;
        if (dev->cursor_y >= VGA_TEXT_ROWS) {
            vga_scroll_up(dev, 1);
            dev->cursor_y = VGA_TEXT_ROWS - 1;
        }
    }
}

static void text_handle_char(vga_device_t* dev, char c)
{
    switch (c) {
    case '\n':
        dev->cursor_x = 0;
        dev->cursor_y++;
        if (dev->cursor_y >= VGA_TEXT_ROWS) {
            vga_scroll_up(dev, 1);
            dev->cursor_y = VGA_TEXT_ROWS - 1;
        }
        break;
    case '\r':
        dev->cursor_x = 0;
        break;
    case '\t':
        dev->cursor_x = (uint16_t)((dev->cursor_x + 8) & ~7u);
        if (dev->cursor_x >= VGA_TEXT_COLS) {
            dev->cursor_x = 0;
            dev->cursor_y++;
            if (dev->cursor_y >= VGA_TEXT_ROWS) {
                vga_scroll_up(dev, 1);
                dev->cursor_y = VGA_TEXT_ROWS - 1;
            }
        }
        break;
    case '\b':
        if (dev->cursor_x > 0) {
            dev->cursor_x--;
            text_write_cell(dev, dev->cursor_x, dev->cursor_y, ' ', dev->cur_attr);
        }
        break;
    default:
        text_write_cell(dev, dev->cursor_x, dev->cursor_y, c, dev->cur_attr);
        text_advance_cursor(dev);
        break;
    }
}

/* =========================================================================
 * Public low-level helpers
 * ========================================================================= */

void vga_putchar_at(vga_device_t* dev, uint16_t x, uint16_t y, char c, uint8_t attr)
{
    if (!dev || !dev->open)
        return;
    if (x >= VGA_TEXT_COLS || y >= VGA_TEXT_ROWS)
        return;
    text_write_cell(dev, x, y, c, attr);
}

void vga_set_cursor_pos(vga_device_t* dev, uint16_t x, uint16_t y)
{
    if (!dev || !dev->open)
        return;
    if (x >= VGA_TEXT_COLS)
        x = VGA_TEXT_COLS - 1;
    if (y >= VGA_TEXT_ROWS)
        y = VGA_TEXT_ROWS - 1;
    dev->cursor_x = x;
    dev->cursor_y = y;
    hw_set_cursor((uint16_t)(y * VGA_TEXT_COLS + x));
}

void vga_show_cursor(vga_device_t* dev, int visible)
{
    if (!dev || !dev->open)
        return;
    dev->cursor_visible = visible;
    hw_cursor_visible(visible);
}

void vga_scroll_up(vga_device_t* dev, int lines)
{
    if (!dev || !dev->open || lines <= 0)
        return;
    if (lines >= VGA_TEXT_ROWS) {
        vga_clear_screen(dev, dev->cur_attr);
        return;
    }
    /* Shift rows up */
    size_t row_words = VGA_TEXT_COLS;
    for (int row = 0; row < VGA_TEXT_ROWS - lines; row++) {
        volatile uint16_t* dst = dev->text_buf + row * row_words;
        volatile uint16_t* src = dev->text_buf + (row + lines) * row_words;
        for (size_t i = 0; i < row_words; i++)
            dst[i] = src[i];
    }
    /* Clear vacated bottom rows */
    uint16_t blank = (uint16_t)((dev->cur_attr << 8) | ' ');
    for (int row = VGA_TEXT_ROWS - lines; row < VGA_TEXT_ROWS; row++) {
        volatile uint16_t* p = dev->text_buf + row * row_words;
        for (size_t i = 0; i < row_words; i++)
            p[i] = blank;
    }
}

void vga_clear_screen(vga_device_t* dev, uint8_t attr)
{
    if (!dev || !dev->open)
        return;
    if (dev->mode == VGA_MODE_TEXT_80x25) {
        uint16_t blank = (uint16_t)((attr << 8) | ' ');
        for (int i = 0; i < VGA_TEXT_COLS * VGA_TEXT_ROWS; i++)
            dev->text_buf[i] = blank;
        dev->cursor_x = 0;
        dev->cursor_y = 0;
        hw_set_cursor(0);
    }
    else {
        /* Graphics mode: fill framebuffer with colour index in attr */
        memset((void*)dev->gfx_buf, attr, (size_t)(dev->width * dev->height));
    }
}

void vga_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, (uint8_t)(r >> 2)); /* 8-bit → 6-bit */
    outb(VGA_DAC_DATA, (uint8_t)(g >> 2));
    outb(VGA_DAC_DATA, (uint8_t)(b >> 2));
}

void vga_get_palette_entry(uint8_t index, uint8_t* r, uint8_t* g, uint8_t* b)
{
    outb(VGA_DAC_READ_INDEX, index);
    *r = (uint8_t)(inb(VGA_DAC_DATA) << 2);
    *g = (uint8_t)(inb(VGA_DAC_DATA) << 2);
    *b = (uint8_t)(inb(VGA_DAC_DATA) << 2);
}

void vga_wait_vsync(void)
{
    /* Wait for end of any current retrace */
    while (inb(VGA_INSTAT1_READ) & 0x08) {
    }
    /* Wait for start of next retrace */
    while (!(inb(VGA_INSTAT1_READ) & 0x08)) {
    }
}

void vga_set_pixel(vga_device_t* dev, uint16_t x, uint16_t y, uint8_t colour)
{
    if (!dev || !dev->open || dev->mode != VGA_MODE_GRAPH_320x200)
        return;
    if (x >= dev->width || y >= dev->height)
        return;
    dev->gfx_buf[(size_t)y * dev->width + x] = colour;
}

uint8_t vga_get_pixel(vga_device_t* dev, uint16_t x, uint16_t y)
{
    if (!dev || !dev->open || dev->mode != VGA_MODE_GRAPH_320x200)
        return 0;
    if (x >= dev->width || y >= dev->height)
        return 0;
    return dev->gfx_buf[(size_t)y * dev->width + x];
}

/* =========================================================================
 * Font loading (plane-2 technique)
 * ========================================================================= */

int vga_load_font(const vga_font_t* font)
{
    if (!font || font->height == 0 || font->height > 32)
        return -EINVAL;

    /* -- Switch sequencer to write to plane 2 only -- */
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x01);
    outb(VGA_SEQ_INDEX, VGA_SEQ_PLANE_WRITE);
    outb(VGA_SEQ_DATA, 0x04);
    outb(VGA_SEQ_INDEX, VGA_SEQ_MEM_MODE);
    outb(VGA_SEQ_DATA, 0x07);
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x03);

    /* -- Graphics Controller: read plane 2, sequential (not odd/even) -- */
    outb(VGA_GC_INDEX, VGA_GC_READ_MAP);
    outb(VGA_GC_DATA, 0x02);
    outb(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, VGA_GC_MISC);
    outb(VGA_GC_DATA, 0x04);

    /* Write 256 glyphs into plane 2 at 0xA0000 */
    volatile uint8_t* plane2 = (volatile uint8_t*)VGA_VRAM_BASE;
    for (int g = 0; g < 256; g++) {
        for (int row = 0; row < font->height; row++) {
            plane2[g * 32 + row] = font->data[g * 32 + row];
        }
        /* Pad remaining scanlines with zeros */
        for (int row = font->height; row < 32; row++) {
            plane2[g * 32 + row] = 0x00;
        }
    }

    /* -- Restore normal text mode sequencer / GC settings -- */
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x01);
    outb(VGA_SEQ_INDEX, VGA_SEQ_PLANE_WRITE);
    outb(VGA_SEQ_DATA, 0x03);
    outb(VGA_SEQ_INDEX, VGA_SEQ_MEM_MODE);
    outb(VGA_SEQ_DATA, 0x03);
    outb(VGA_SEQ_INDEX, VGA_SEQ_RESET);
    outb(VGA_SEQ_DATA, 0x03);

    outb(VGA_GC_INDEX, VGA_GC_READ_MAP);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    outb(VGA_GC_DATA, 0x10);
    outb(VGA_GC_INDEX, VGA_GC_MISC);
    outb(VGA_GC_DATA, 0x0E);

    /* Update CRTC max scanline for new font height */
    outb(VGA_CRTC_INDEX, VGA_CRTC_MAX_SCAN);
    uint8_t ms = inb(VGA_CRTC_DATA);
    ms         = (uint8_t)((ms & 0xE0) | ((font->height - 1) & 0x1F));
    outb(VGA_CRTC_DATA, ms);

    return 0;
}

/* =========================================================================
 * Mode setting
 * ========================================================================= */

int vga_set_mode(vga_device_t* dev, vga_mode_t mode)
{
    if (!dev)
        return -ENODEV;

    switch (mode) {
    case VGA_MODE_TEXT_80x25:
        write_regs(&g_regs_text80x25);
        dev->mode     = VGA_MODE_TEXT_80x25;
        dev->width    = VGA_TEXT_COLS;
        dev->height   = VGA_TEXT_ROWS;
        dev->bpp      = 0;
        dev->text_buf = (volatile uint16_t*)VGA_TEXT_BASE;
        dev->gfx_buf  = NULL;
        dev->cursor_x = 0;
        dev->cursor_y = 0;
        hw_set_cursor(0);
        hw_cursor_visible(dev->cursor_visible);
        break;

    case VGA_MODE_GRAPH_320x200:
        write_regs(&g_regs_mode13h);
        dev->mode     = VGA_MODE_GRAPH_320x200;
        dev->width    = 320;
        dev->height   = 200;
        dev->bpp      = 8;
        dev->gfx_buf  = (volatile uint8_t*)VGA_VRAM_BASE;
        dev->text_buf = NULL;

        /* Allocate / reallocate shadow buffer */
        if (dev->shadow_buf)
            kfree(dev->shadow_buf);
        dev->shadow_size = (size_t)(dev->width * dev->height);
        dev->shadow_buf  = (uint8_t*)kmalloc(dev->shadow_size);
        if (!dev->shadow_buf) {
            dev->shadow_size = 0;
            /* Non-fatal: shadow buffering unavailable */
        }
        else {
            memset(dev->shadow_buf, 0, dev->shadow_size);
        }
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

int vga_init()
{
    device_t* dev;
    int       res = device_misc_create(&__driver_vga, &dev);
    if (res)
        return res;
    vga_open(dev);
    res = vfs_register_device(dev);
    if (res)
        return res;
    device_t* tty_dev = vfs_get_device("tty0");
    if (tty_dev) {
        // Set VGA device as output for the first TTY
        tty_ioctl(tty_dev, TTY_IOCTL_SETOUTPUTDEV, dev);
    }
    else {
        printf("Warning: No TTY device found to set VGA output\n");
    }

    return 0;
}
/* =========================================================================
 * VGA driver implementation
 * ========================================================================= */

int vga_probe(device_t* dev)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = kmalloc(sizeof(vga_device_t));
    if (!vdev)
        return -ENOMEM;

    memset(vdev, 0, sizeof(*vdev));
    dev->softc = vdev;
    return 0;
}

int vga_attach(device_t* dev)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = kmalloc(sizeof(vga_device_t));
    if (!vdev)
        return -ENODEV;

    dev->softc = vdev;

    memset(vdev, 0, sizeof(*vdev));
    vdev->cur_attr       = VGA_ATTR(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vdev->cursor_visible = 1;
    vdev->open           = 1;
    vdev->ref_count      = 1;

    int rc = vga_set_mode(vdev, VGA_MODE_TEXT_80x25);
    if (rc) {
        vdev->open = 0;
        return rc;
    }

    // vga_clear_screen(vdev, vdev->cur_attr);
    return 0;
}

int vga_detach(device_t* dev)
{
    if (!dev)
        return -ENODEV;
    /* Resources are cleaned up in close() */
    return 0;
}

int vga_suspend(device_t* dev)
{
    return 0;
}

int vga_resume(device_t* dev)
{
    return 0;
}

int vga_shutdown(device_t* dev)
{
    return 0;
}

/* ========================================================================
 * vga device ops: open, close, read, write, ioctl
 * ========================================================================= */

int vga_open(device_t* dev)
{
    return 0;
}

int vga_close(device_t* dev)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = (vga_device_t*)dev->softc;
    if (!vdev || !vdev->open)
        return -EBADF;

    if (--vdev->ref_count > 0)
        return 0;

    /* Restore text mode */
    vga_set_mode(vdev, VGA_MODE_TEXT_80x25);
    vga_clear_screen(vdev, VGA_ATTR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    vga_show_cursor(vdev, 1);

    if (vdev->shadow_buf) {
        kfree(vdev->shadow_buf);
        vdev->shadow_buf  = NULL;
        vdev->shadow_size = 0;
    }

    vdev->open      = 0;
    vdev->ref_count = 0;
    return 0;
}

/* =========================================================================
 * vga_read — read bytes from the framebuffer
 * ========================================================================= */

ssize_t vga_read(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buf)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = (vga_device_t*)dev->softc;
    if (!vdev || !vdev->open)
        return -EBADF;
    if (!buf)
        return -EFAULT;
    if (size == 0)
        return 0;

    if (vdev->mode == VGA_MODE_TEXT_80x25) {
        /* Text framebuffer: 2 bytes per cell, total VGA_TEXT_BUF_SIZE */
        size_t fb_size = VGA_TEXT_BUF_SIZE;
        if (offset >= fb_size)
            return 0;
        if (offset + size > fb_size)
            size = fb_size - offset;

        const uint8_t* src = (const uint8_t*)vdev->text_buf + offset;
        uint8_t*       dst = (uint8_t*)buf;
        for (size_t i = 0; i < size; i++)
            dst[i] = src[i];

        return (ssize_t)size;
    }
    else {
        /* Graphics mode: read from shadow buffer if available, else VRAM */
        size_t fb_size = (size_t)(vdev->width * vdev->height);
        if (offset >= fb_size)
            return 0;
        if (offset + size > fb_size)
            size = fb_size - offset;

        const uint8_t* src =
            vdev->shadow_buf ? vdev->shadow_buf + offset : (const uint8_t*)vdev->gfx_buf + offset;
        memcpy(buf, src, size);
        return (ssize_t)size;
    }
}

/* =========================================================================
 * vga_write — write bytes to the framebuffer / text console
 * ========================================================================= */

int vga_write(device_t* dev, uint64_t offset, uint32_t count, const uint8_t* buf)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = (vga_device_t*)dev->softc;
    if (!vdev || !vdev->open)
        return -EBADF;
    if (!buf)
        return -EFAULT;
    if (count == 0)
        return 0;

    if (vdev->mode == VGA_MODE_TEXT_80x25) {
        /*
         * Text mode: interpret buf as a stream of ASCII characters,
         * processed through the console logic (handles \n, \r, \t, \b).
         * 'offset' is ignored in text mode; the cursor position is used.
         */
        const char* chars = (const char*)buf;
        for (size_t i = 0; i < count; i++) {
            text_handle_char(vdev, chars[i]);
        }
        /* Update hardware cursor */
        hw_set_cursor((uint16_t)(vdev->cursor_y * VGA_TEXT_COLS + vdev->cursor_x));
        return (ssize_t)count;
    }
    else {
        /*
         * Graphics mode: raw pixel data written to the framebuffer.
         * If buf starts with a vga_write_header_t, blit a rectangle;
         * otherwise write sequentially starting at 'offset'.
         */
        size_t fb_size = (size_t)(vdev->width * vdev->height);

        if (count >= sizeof(vga_write_header_t)) {
            const vga_write_header_t* hdr = (const vga_write_header_t*)buf;
            /* Validate magic: check if coordinates are plausible */
            if (hdr->x < vdev->width && hdr->y < vdev->height && hdr->w > 0 && hdr->h > 0) {
                const uint8_t* pixels  = (const uint8_t*)buf + sizeof(vga_write_header_t);
                size_t         payload = count - sizeof(vga_write_header_t);
                size_t         needed  = (size_t)(hdr->w * hdr->h);
                if (payload < needed)
                    needed = payload;

                size_t written = 0;
                for (uint16_t row = 0; row < hdr->h && row + hdr->y < vdev->height; row++) {
                    for (uint16_t col = 0; col < hdr->w && col + hdr->x < vdev->width; col++) {
                        if (written >= needed)
                            goto done_blit;
                        size_t  idx = (size_t)((hdr->y + row) * vdev->width + (hdr->x + col));
                        uint8_t pix = pixels[written++];
                        vdev->gfx_buf[idx] = pix;
                        if (vdev->shadow_buf)
                            vdev->shadow_buf[idx] = pix;
                    }
                }
            done_blit:
                return (ssize_t)(sizeof(vga_write_header_t) + written);
            }
        }

        /* Sequential raw write at offset */
        if (offset >= fb_size)
            return 0;
        if (offset + count > fb_size)
            count = fb_size - offset;

        const uint8_t* src = (const uint8_t*)buf;
        for (size_t i = 0; i < count; i++) {
            vdev->gfx_buf[offset + i] = src[i];
            if (vdev->shadow_buf)
                vdev->shadow_buf[offset + i] = src[i];
        }
        return (ssize_t)count;
    }
}

/* =========================================================================
 * vga_ioctl — device control
 * ========================================================================= */

int vga_ioctl(device_t* dev, int cmd, void* arg)
{
    if (!dev)
        return -ENODEV;

    vga_device_t* vdev = (vga_device_t*)dev->softc;
    if (!vdev || !vdev->open)
        return -EBADF;

    switch (cmd) {

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_SET_MODE: {
        vga_mode_t mode = (vga_mode_t)arg;
        return vga_set_mode(vdev, mode);
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_GET_MODE: {
        vga_mode_t* out = (vga_mode_t*)arg;
        if (!out)
            return -EFAULT;
        *out = vdev->mode;
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_SET_CURSOR: {
        const vga_cursor_t* cur = (const vga_cursor_t*)arg;
        if (!cur)
            return -EFAULT;
        vga_set_cursor_pos(vdev, cur->x, cur->y);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_GET_CURSOR: {
        vga_cursor_t* cur = (vga_cursor_t*)arg;
        if (!cur)
            return -EFAULT;
        cur->x = vdev->cursor_x;
        cur->y = vdev->cursor_y;
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_CURSOR_SHOW: {
        vga_show_cursor(vdev, (int)arg);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_SET_PALETTE: {
        const vga_palette_entry_t* ent = (const vga_palette_entry_t*)arg;
        if (!ent)
            return -EFAULT;
        vga_set_palette_entry(ent->index, ent->r, ent->g, ent->b);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_GET_PALETTE: {
        vga_palette_entry_t* ent = (vga_palette_entry_t*)arg;
        if (!ent)
            return -EFAULT;
        vga_get_palette_entry(ent->index, &ent->r, &ent->g, &ent->b);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_LOAD_PALETTE: {
        const vga_palette_t* pal = (const vga_palette_t*)arg;
        if (!pal)
            return -EFAULT;
        for (int i = 0; i < 256; i++) {
            vga_set_palette_entry(pal->entries[i].index, pal->entries[i].r, pal->entries[i].g,
                                  pal->entries[i].b);
        }
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_CLEAR: {
        uint8_t attr = (uint8_t)((uintptr_t)arg & 0xFF);
        /* If arg is 0, use current attribute */
        if (!arg)
            attr = vdev->cur_attr;
        vga_clear_screen(vdev, attr);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_SCROLL: {
        int lines = (int)(intptr_t)arg;
        if (lines < 0)
            return -EINVAL; /* scroll-down not yet implemented */
        vga_scroll_up(vdev, lines);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_SET_ATTR: {
        vdev->cur_attr = (uint8_t)((uintptr_t)arg & 0xFF);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_GET_INFO: {
        vga_info_t* info = (vga_info_t*)arg;
        if (!info)
            return -EFAULT;
        info->mode      = vdev->mode;
        info->width     = vdev->width;
        info->height    = vdev->height;
        info->bpp       = vdev->bpp;
        info->text_mode = (vdev->mode == VGA_MODE_TEXT_80x25) ? 1 : 0;
        return 0;
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_LOAD_FONT: {
        const vga_font_t* font = (const vga_font_t*)arg;
        if (!font)
            return -EFAULT;
        return vga_load_font(font);
    }

    /* ------------------------------------------------------------------ */
    case VGA_IOCTL_WAIT_VSYNC: {
        vga_wait_vsync();
        return 0;
    }

    default:
        return -EINVAL;
    }
}
