#include "kernel/interrupts/idt.h"
#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"

// The IDT and IDT pointer
struct idt_entry_t idt[IDT_SIZE];
struct idt_ptr_t ip;

// Function to set an IDT entry
void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = base & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attributes = type_attr;
    idt[index].offset_high = (base >> 16) & 0xFFFF;
}

void remap_pic() {
	outb(0x20, 0x11); // Master PIC command
	outb(0xA0, 0x11); // Slave PIC command
	outb(0x21, 0x20); // Data PIC offset
	outb(0xA1, 0x28); // Slave PIC offset
	outb(0x21, 0x04); // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(0xA1, 0x02); // Tell Slave PIC its cascade identity (0000 0010)
	outb(0x21, 0x01); // 8086/88 (MCS-80/85) mode
	outb(0xA1, 0x01); // 8086/88 (MCS-80/85) mode

	outb(0x21, 0x0);  // Mask all interrupts except IRQ1
	outb(0xA1, 0x0);  // Mask all interrupts
}

// Load the IDT into the CPU using the LIDT instruction
void load_idt() {
    ip.limit = (sizeof(struct idt_entry_t) * IDT_SIZE) - 1;
    ip.base = (uint32_t)&idt;

    asm volatile("lidt (%0)" : : "r"(&ip));
}

// Initialize the IDT with the keyboard ISR and default handlers
void idt_init() {
    // Set up the default handler for all interrupts (for unused vectors)
    for (int i = 0; i < IDT_SIZE; ++i) {
        set_idt_entry(i, 0, 0x08, 0x8E);  // Interrupt gate, present
    }

    // Set up the keyboard ISR (IRQ1 -> Interrupt vector 33)
    set_idt_entry(0x21, (uint32_t)keyboard_isr_wrapper, 0x08, 0x8E);  // Interrupt gate, present
	// Set up the page fault ISR (Interrupt vector 14)
	set_idt_entry(0x0E, (uint32_t)page_fault_isr_wrapper, 0x08, 0x8E);  // Interrupt gate, present

    // Load the IDT into the CPU
    load_idt();

	// Remap the PIC
	remap_pic();
}