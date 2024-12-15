#include "kernel/interrupts/idt.h"
#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include "kernel/terminal.h"

void main(void) {
    // Initialize the terminal
    terminal_initialize();
    terminal_print("Hello, kernel World!\n");
    initialize_idt();

    // Enable interrupts
    terminal_print("Enabling interrupts\n");
    asm volatile("sti");
    terminal_print("Interrupts enabled\n");

    while (1) {
        //terminal_putchar('a');
    }
}