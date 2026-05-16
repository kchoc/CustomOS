#ifndef _I386_BOOTINFO_H_
#define _I386_BOOTINFO_H_

#include <vm/vm_phys.h>

#include <inttypes.h>

typedef struct bootinfo_t {
    memory_map_entry_t* memory_map;
    uint32_t            memory_map_length;
    uint32_t            framebuffer_type; // 0 = none, 1 = VGA text mode, 2 = VBE framebuffer
    uint32_t            framebuffer_addr;
    uint32_t            framebuffer_pitch;
    uint32_t            framebuffer_width;
    uint32_t            framebuffer_height;
    uint32_t            framebuffer_bpp;
    uint32_t            framebuffer_red_position;
    uint32_t            framebuffer_red_mask;
    uint32_t            framebuffer_green_position;
    uint32_t            framebuffer_green_mask;
    uint32_t            framebuffer_blue_position;
    uint32_t            framebuffer_blue_mask;
} bootinfo_t;

extern bootinfo_t* bootinfo;

#endif // _I386_BOOTINFO_H_
