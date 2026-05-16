#include "display.h"

#include <dev/vbe/vbe.h>
#include <dev/vga/vga.h>

#include <kern/terminal.h>
#include <kern/panic.h>

void display_init(void)
{
    if (bootinfo->framebuffer_type == 0) {
        printf("Display: No framebuffer detected\n");
        return;
    }

    if (bootinfo->framebuffer_type == 1) {
        printf("Display: VGA text mode detected\n");
        vga_init();
    } else if (bootinfo->framebuffer_type == 2) {
        printf("Display: VBE framebuffer detected\n");
        vbe_init(); 
    } else {
        PANIC("Unknown framebuffer type detected");
    }
}
