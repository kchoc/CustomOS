#ifndef VGA_H
#define VGA_H

#include "kernel/types.h"
#include "kernel/wm/display.h"

/* VGA Mode 13h */
#define VGA_WIDTH  SCREEN_WIDTH
#define VGA_HEIGHT SCREEN_HEIGHT
#define VGA_MEMORY 0xA0000

void vga_set_mode_13h(void);
void vga_set_mode_text(void);

#endif // VGA_H
