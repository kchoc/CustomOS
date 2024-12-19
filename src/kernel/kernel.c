#include "kernel/interrupts/idt.h"
#include "kernel/drivers/port_io.h"
#include "kernel/terminal.h"

#include "kernel/memory/gdt.h"
#include "kernel/memory/page.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/layout.h"

void main() {
    terminal_init();
    printf("Terminal initialized\n");

    page_init();
    initialize_paging();
    printf("Paging initialized\n");

    kmalloc_init((char *) KMALLOC_START, KMALLOC_SIZE);
    printf("kmalloc initialized\n");

    gdt_init();
    printf("GDT initialized\n");

    idt_init();
    printf("IDT initialized\n");

    // Enable interrupts
    outb(0x21, 0xFD); // Mask all interrupts except IRQ1
    asm volatile("sti");
    printf("Interrupts enabled\n");

    while (1) {
        // Do nothing
    }
}