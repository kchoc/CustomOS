#include "vbe.h"
#include <bootinfo.h>

#include <dev/port/port_io.h>
#include <kern/terminal.h>
#include <string.h>
#include <vm/vm_map.h>

static vbe_mode_info_t current_mode_info   = {0};
static uint32_t*       framebuffer_virtual = 0;

int vbe_init(void)
{
    if (bootinfo->framebuffer_type != 2) {
        printf("VBE: No VBE framebuffer detected\n");
        return -1;
    }

    // Copy mode info from bootloader
    current_mode_info.width       = bootinfo->framebuffer_width;
    current_mode_info.height      = bootinfo->framebuffer_height;
    current_mode_info.bpp         = bootinfo->framebuffer_bpp;
    current_mode_info.pitch       = bootinfo->framebuffer_pitch;
    current_mode_info.framebuffer = vm_map_device(
        bootinfo->framebuffer_addr,
        current_mode_info.width * current_mode_info.height * (current_mode_info.bpp / 8),
        VM_PROT_READ | VM_PROT_WRITE, 0);

    return 0;
}

int vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp)
{
    // This would require real mode BIOS calls (INT 0x10, AX=0x4F02)
    // For now, we just use what the bootloader gave us
    printf("VBE: Mode switching not implemented (using bootloader mode)\n");
    return -1;
}

uint32_t vbe_get_framebuffer(void)
{
    return current_mode_info.framebuffer;
}

uint32_t* vbe_get_framebuffer_virtual(void)
{
    return framebuffer_virtual;
}

uint16_t vbe_get_pitch(void)
{
    return current_mode_info.pitch;
}

uint16_t vbe_get_width(void)
{
    return current_mode_info.width;
}

uint16_t vbe_get_height(void)
{
    return current_mode_info.height;
}

uint8_t vbe_get_bpp(void)
{
    return current_mode_info.bpp;
}
