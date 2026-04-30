#ifndef VGA_DEVICE_H
#define VGA_DEVICE_H

#include <sys/device.h>

#include <stdint.h>
#include <stddef.h>

/* =========================================================================
 * VGA I/O Port Definitions
 * ========================================================================= */

/* General / Miscellaneous */
#define VGA_MISC_WRITE          0x3C2
#define VGA_MISC_READ           0x3CC
#define VGA_FEATURE_WRITE       0x3DA   /* also Input Status 1 (read) */
#define VGA_INSTAT1_READ        0x3DA

/* Sequencer */
#define VGA_SEQ_INDEX           0x3C4
#define VGA_SEQ_DATA            0x3C5

#define VGA_SEQ_RESET           0x00
#define VGA_SEQ_CLOCKING        0x01
#define VGA_SEQ_PLANE_WRITE     0x02
#define VGA_SEQ_CHAR_MAP        0x03
#define VGA_SEQ_MEM_MODE        0x04

/* CRT Controller */
#define VGA_CRTC_INDEX          0x3D4
#define VGA_CRTC_DATA           0x3D5

#define VGA_CRTC_HTOTAL         0x00
#define VGA_CRTC_HDISPLAY_END   0x01
#define VGA_CRTC_HBLANK_START   0x02
#define VGA_CRTC_HBLANK_END     0x03
#define VGA_CRTC_HRETRACE_START 0x04
#define VGA_CRTC_HRETRACE_END   0x05
#define VGA_CRTC_VTOTAL         0x06
#define VGA_CRTC_OVERFLOW       0x07
#define VGA_CRTC_PRESET_ROW     0x08
#define VGA_CRTC_MAX_SCAN       0x09
#define VGA_CRTC_CURSOR_HI      0x0A
#define VGA_CRTC_CURSOR_LO      0x0B
#define VGA_CRTC_START_HI       0x0C
#define VGA_CRTC_START_LO       0x0D
#define VGA_CRTC_CURSOR_LOC_HI  0x0E
#define VGA_CRTC_CURSOR_LOC_LO  0x0F
#define VGA_CRTC_VRETRACE_START 0x10
#define VGA_CRTC_VRETRACE_END   0x11
#define VGA_CRTC_VDISPLAY_END   0x12
#define VGA_CRTC_OFFSET         0x13
#define VGA_CRTC_UNDERLINE      0x14
#define VGA_CRTC_VBLANK_START   0x15
#define VGA_CRTC_VBLANK_END     0x16
#define VGA_CRTC_MODE_CTRL      0x17
#define VGA_CRTC_LINE_COMPARE   0x18

/* Graphics Controller */
#define VGA_GC_INDEX            0x3CE
#define VGA_GC_DATA             0x3CF

#define VGA_GC_SET_RESET        0x00
#define VGA_GC_ENABLE_SR        0x01
#define VGA_GC_COLOR_COMPARE    0x02
#define VGA_GC_DATA_ROTATE      0x03
#define VGA_GC_READ_MAP         0x04
#define VGA_GC_GRAPHICS_MODE    0x05
#define VGA_GC_MISC             0x06
#define VGA_GC_COLOR_DONT_CARE  0x07
#define VGA_GC_BIT_MASK         0x08

/* Attribute Controller */
#define VGA_AC_INDEX            0x3C0
#define VGA_AC_WRITE            0x3C0
#define VGA_AC_READ             0x3C1

#define VGA_AC_PALETTE0         0x00    /* through 0x0F */
#define VGA_AC_MODE_CTRL        0x10
#define VGA_AC_OVERSCAN         0x11
#define VGA_AC_COLOR_PLANE_EN   0x12
#define VGA_AC_HPAN             0x13
#define VGA_AC_COLOR_SELECT     0x14

/* DAC / Palette */
#define VGA_DAC_READ_INDEX      0x3C7
#define VGA_DAC_WRITE_INDEX     0x3C8
#define VGA_DAC_DATA            0x3C9
#define VGA_DAC_STATE           0x3C6   /* PEL mask */

/* =========================================================================
 * Video Memory Layout
 * ========================================================================= */

#define VGA_VRAM_BASE           0xA0000
#define VGA_VRAM_SIZE           0x20000   /* 128 KB */
#define VGA_TEXT_BASE           0xC03FF000
#define VGA_TEXT_SIZE           0x8000    /* 32 KB  */
#define VGA_MONO_BASE           0xB0000

/* =========================================================================
 * Text Mode Constants
 * ========================================================================= */

#define VGA_TEXT_COLS           80
#define VGA_TEXT_ROWS           25
#define VGA_TEXT_BUF_SIZE       (VGA_TEXT_COLS * VGA_TEXT_ROWS * 2)

/* Text attribute byte helpers */
#define VGA_ATTR(fg, bg)        (((bg) << 4) | ((fg) & 0x0F))
#define VGA_ATTR_BLINK(fg, bg)  (((bg) << 4) | ((fg) & 0x0F) | 0x80)

/* Standard 16 colours (foreground / low-nibble background) */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,  /* "Yellow" */
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* =========================================================================
 * Video Mode Definitions
 * ========================================================================= */

typedef enum {
    VGA_MODE_TEXT_80x25   = 0x03,   /* Standard colour text    */
    VGA_MODE_GRAPH_320x200 = 0x13,  /* Mode 13h – 256 colours  */
} vga_mode_t;

/* =========================================================================
 * IOCTL Command Codes
 * ========================================================================= */

#define VGA_IOC_MAGIC           'V'

/* Mode control */
#define VGA_IOCTL_SET_MODE      0x01  /* arg: vga_mode_t          */
#define VGA_IOCTL_GET_MODE      0x02  /* arg: vga_mode_t *        */

/* Cursor */
#define VGA_IOCTL_SET_CURSOR    0x03  /* arg: vga_cursor_t *      */
#define VGA_IOCTL_GET_CURSOR    0x04  /* arg: vga_cursor_t *      */
#define VGA_IOCTL_CURSOR_SHOW   0x05  /* arg: int (bool)          */

/* Palette */
#define VGA_IOCTL_SET_PALETTE   0x06  /* arg: vga_palette_entry_t */
#define VGA_IOCTL_GET_PALETTE   0x07  /* arg: vga_palette_entry_t */
#define VGA_IOCTL_LOAD_PALETTE  0x08  /* arg: vga_palette_t *     */

/* Display */
#define VGA_IOCTL_CLEAR         0x09  /* arg: uint8_t attr        */
#define VGA_IOCTL_SCROLL        0x0A  /* arg: int lines           */
#define VGA_IOCTL_SET_ATTR      0x0B  /* arg: uint8_t attr        */
#define VGA_IOCTL_GET_INFO      0x0C  /* arg: vga_info_t *        */

/* Font */
#define VGA_IOCTL_LOAD_FONT     0x0D  /* arg: vga_font_t *        */

/* Vsync */
#define VGA_IOCTL_WAIT_VSYNC    0x0E  /* no arg                   */

/* =========================================================================
 * IOCTL Argument Structures
 * ========================================================================= */

typedef struct {
    uint16_t x;
    uint16_t y;
} vga_cursor_t;

typedef struct {
    uint8_t index;
    uint8_t r;     /* 0-255, driver scales to 6-bit DAC */
    uint8_t g;
    uint8_t b;
} vga_palette_entry_t;

typedef struct {
    vga_palette_entry_t entries[256];
} vga_palette_t;

typedef struct {
    vga_mode_t  mode;
    uint16_t    width;       /* columns (text) or pixels (graphics) */
    uint16_t    height;      /* rows   (text) or pixels (graphics)  */
    uint8_t     bpp;         /* bits per pixel (text = 0)           */
    uint8_t     text_mode;   /* non-zero if currently in text mode  */
} vga_info_t;

/* Font: up to 256 glyphs, up to 32 bytes tall */
typedef struct {
    uint8_t  height;         /* bytes (scanlines) per glyph         */
    uint8_t  data[256 * 32];
} vga_font_t;

/* =========================================================================
 * Write Packet (used by device write() for graphics mode pixel blitting)
 * ========================================================================= */

/*
 * In text mode, write() accepts raw bytes interpreted as characters using
 * the current attribute.  In graphics mode, callers may optionally prefix
 * data with a vga_write_header_t to specify a destination rectangle.
 * If no header is used, bytes are written sequentially from the current
 * framebuffer offset (lseek controls position).
 */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} __attribute__((packed)) vga_write_header_t;

/* =========================================================================
 * Device State
 * ========================================================================= */

typedef struct {
    vga_mode_t  mode;
    uint16_t    width;
    uint16_t    height;
    uint8_t     bpp;

    /* Text mode state */
    uint16_t    cursor_x;
    uint16_t    cursor_y;
    uint8_t     cur_attr;       /* current text attribute byte      */
    int         cursor_visible;

    /* Mapped framebuffer (kernel virtual address) */
    volatile uint16_t *text_buf;   /* text mode  */
    volatile uint8_t  *gfx_buf;    /* graphics mode */

    /* Shadow / back-buffer for double-buffering (optional) */
    uint8_t    *shadow_buf;
    size_t      shadow_size;

    int         open;           /* non-zero when device is open     */
    int         ref_count;
} vga_device_t;

/* =========================================================================
 * Public Driver API (file-operation style)
 * ========================================================================= */

DECLARE_DEVICE_TYPE(vga);

int vga_init(void);

/* =========================================================================
 * Low-level Helpers (available for use by other kernel subsystems)
 * ========================================================================= */

void vga_putchar_at(vga_device_t *dev, uint16_t x, uint16_t y,
                    char c, uint8_t attr);
void vga_set_cursor_pos(vga_device_t *dev, uint16_t x, uint16_t y);
void vga_show_cursor(vga_device_t *dev, int visible);
void vga_scroll_up(vga_device_t *dev, int lines);
void vga_clear_screen(vga_device_t *dev, uint8_t attr);
void vga_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vga_get_palette_entry(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);
void vga_wait_vsync(void);
int  vga_load_font(const vga_font_t *font);
int  vga_set_mode(vga_device_t *dev, vga_mode_t mode);

/* Pixel operation (graphics modes only) */
void vga_set_pixel(vga_device_t *dev, uint16_t x, uint16_t y, uint8_t colour);
uint8_t vga_get_pixel(vga_device_t *dev, uint16_t x, uint16_t y);

#endif /* VGA_DEVICE_H */

