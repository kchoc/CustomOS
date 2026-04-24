#ifndef DEV_VGA_H
#define DEV_VGA_H

#include <sys/device.h>
#include <sys/tty.h>

DECLARE_DEVICE_TYPE(vga);
DECLARE_CONSOLE_TYPE(vga);

typedef struct vga_device_data {
    uint16_t* framebuffer;
    uint16_t  width;
    uint16_t  height;
    uint16_t  x; // Current cursor x position
    uint16_t  y; // Current cursor y position
} vga_device_data_t;

int vga_init(void);

#endif // DEV_VGA_H
