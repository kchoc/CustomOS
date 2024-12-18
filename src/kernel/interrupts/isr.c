#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/terminal.h"

// The default handler for unhanded interrupts
void isr_default_handler() {
    outb(0x20, 0x20);  // Send an EOI to the PIC
}

// The keyboard ISR handler (IRQ1 -> Interrupt vector 33)
void keyboard_isr_handler() {
    // Read the scan code from the keyboard
    uint8_t scan_code = inb(0x60);

    // Print the scan code
    handle_keypress(scan_code);
    
    // Send an EOI to the PIC
    outb(0x20, 0x20); // Send EOI to PIC1
    outb(0xA0, 0x20); // Send EOI to PIC2
}