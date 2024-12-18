#include "kernel/interrupts/idt.h"
#include "kernel/memory/gdt.h"
#include "kernel/drivers/port_io.h"
#include "kernel/terminal.h"

void main() {
    terminal_initialize();
    terminal_print("Terminal initialized\n");    
    initialize_gdt();
    terminal_print("GDT initialized\n");
    initialize_idt();
    terminal_print("IDT initialized\n");

    // Enable interrupts
    outb(0x21, 0xFD); // Mask all interrupts except IRQ1
    asm volatile("sti");
    terminal_print("Interrupts enabled\n");

    while (1) {
        // Do nothing
    }
}