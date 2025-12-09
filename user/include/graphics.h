#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

/* VGA Mode 13h Constants */
#define GFX_WIDTH  320
#define GFX_HEIGHT 200

/* Graphics Context */
typedef struct {
    volatile uint32_t* framebuffer;  // 32-bit RGBA framebuffer
    int window_id;
    int width;
    int height;
    int is_windowed;
} gfx_context_t;

/* Standard 32-bit RGB Colors */
#define COLOR_BLACK         0x000000
#define COLOR_BLUE          0x0000AA
#define COLOR_GREEN         0x00AA00
#define COLOR_CYAN          0x00AAAA
#define COLOR_RED           0xAA0000
#define COLOR_MAGENTA       0xAA00AA
#define COLOR_BROWN         0xAA5500
#define COLOR_LIGHT_GREY    0xAAAAAA
#define COLOR_DARK_GREY     0x555555
#define COLOR_LIGHT_BLUE    0x5555FF
#define COLOR_LIGHT_GREEN   0x55FF55
#define COLOR_LIGHT_CYAN    0x55FFFF
#define COLOR_LIGHT_RED     0xFF5555
#define COLOR_LIGHT_MAGENTA 0xFF55FF
#define COLOR_YELLOW        0xFFFF55
#define COLOR_WHITE         0xFFFFFF

/* Context Management */
gfx_context_t* gfx_create_context(int wid, int width, int height);
void gfx_destroy_context(gfx_context_t* ctx);
void gfx_set_context(gfx_context_t* ctx);
gfx_context_t* gfx_get_context(void);

/* Graphics Functions - Context-aware (use current context) */
void gfx_init(void);  // Must be called before using any graphics functions (framebuffer mode)
void gfx_init_window(int wid, int width, int height);  // Initialize for windowed mode
void gfx_put_pixel(int x, int y, uint32_t color);
uint32_t gfx_get_pixel(int x, int y);
void gfx_clear_screen(uint32_t color);
void gfx_fill_rect(int x, int y, int width, int height, uint32_t color);
void gfx_draw_rect(int x, int y, int width, int height, uint32_t color);
void gfx_draw_hline(int x, int y, int width, uint32_t color);
void gfx_draw_vline(int x, int y, int height, uint32_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_draw_string(int x, int y, const char* str, uint32_t fg, uint32_t bg);
void gfx_flush(void);  // Flush window buffer to screen (windowed mode only)

/* Context-explicit Functions (specify context directly) */
void gfx_put_pixel_ctx(gfx_context_t* ctx, int x, int y, uint32_t color);
uint32_t gfx_get_pixel_ctx(gfx_context_t* ctx, int x, int y);
void gfx_clear_screen_ctx(gfx_context_t* ctx, uint32_t color);
void gfx_fill_rect_ctx(gfx_context_t* ctx, int x, int y, int width, int height, uint32_t color);
void gfx_draw_rect_ctx(gfx_context_t* ctx, int x, int y, int width, int height, uint32_t color);
void gfx_draw_hline_ctx(gfx_context_t* ctx, int x, int y, int width, uint32_t color);
void gfx_draw_vline_ctx(gfx_context_t* ctx, int x, int y, int height, uint32_t color);
void gfx_draw_line_ctx(gfx_context_t* ctx, int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_char_ctx(gfx_context_t* ctx, int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_draw_string_ctx(gfx_context_t* ctx, int x, int y, const char* str, uint32_t fg, uint32_t bg);
void gfx_flush_ctx(gfx_context_t* ctx);

#endif // GRAPHICS_H
