#include "kernel/interrupts/idt.h"

// The IDT and IDT pointer
idt_entry_t idt[IDT_SIZE];
idt_ptr_t idt_ptr;

// Function to set an IDT entry
void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr) {
    idt[index].base_low = base & 0xFFFF;
    idt[index].base_high = (base >> 16) & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type_attr;
}

// Load the IDT into the CPU using the LIDT instruction
void load_idt() {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_SIZE) - 1;
    idt_ptr.base = (uint32_t)&idt;

    asm volatile("lidt %0" : : "m"(idt_ptr));
}

// Initialize the IDT with the keyboard ISR and default handlers
void initialize_idt() {
    // Set up the default handler for all interrupts (for unused vectors)
    for (int i = 0; i < IDT_SIZE; ++i) {
        set_idt_entry(i, (uint32_t)isr_default_handler, 0x08, 0x8E);  // Interrupt gate, present
    }

    // Set up the keyboard ISR (IRQ1 -> Interrupt vector 33)
    set_idt_entry(33, (uint32_t)keyboard_isr_handler, 0x08, 0x8E);  // Interrupt gate, present

    // Load the IDT into the CPU
    load_idt();
}