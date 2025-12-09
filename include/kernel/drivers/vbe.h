#ifndef VBE_H
#define VBE_H

#include "kernel/types.h"

/* VESA VBE Mode Info Block */
typedef struct {
    uint16_t attributes;
    uint8_t  window_a;
    uint8_t  window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t  w_char;
    uint8_t  y_char;
    uint8_t  planes;
    uint8_t  bpp;
    uint8_t  banks;
    uint8_t  memory_model;
    uint8_t  bank_size;
    uint8_t  image_pages;
    uint8_t  reserved0;
    
    uint8_t  red_mask;
    uint8_t  red_position;
    uint8_t  green_mask;
    uint8_t  green_position;
    uint8_t  blue_mask;
    uint8_t  blue_position;
    uint8_t  reserved_mask;
    uint8_t  reserved_position;
    uint8_t  direct_color_attributes;
    
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

/* VBE Info Block */
typedef struct {
    char     signature[4];    // "VESA"
    uint16_t version;
    uint32_t oem_string_ptr;
    uint32_t capabilities;
    uint32_t video_modes_ptr;
    uint16_t total_memory;
    uint8_t  reserved[492];
} __attribute__((packed)) vbe_info_t;

/* VBE Functions */
int vbe_init(void);
int vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp);
uint32_t vbe_get_framebuffer(void);
uint32_t* vbe_get_framebuffer_virtual(void);
uint16_t vbe_get_pitch(void);
uint16_t vbe_get_width(void);
uint16_t vbe_get_height(void);
uint8_t vbe_get_bpp(void);

#endif // VBE_H
