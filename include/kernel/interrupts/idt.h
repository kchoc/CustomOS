#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Define the number of entries in the IDT
#define IDT_SIZE 256

// IDT entry structure
struct idt_entry_t {
    uint16_t offset_low;      // Offset bits 0-15
    uint16_t selector;       // Code segment selector
    uint8_t zero;            // Unused, Always 0
    uint8_t type_attributes; // Gate type and attributes
    uint16_t offset_high;    // Offset bits 16-31
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void keyboard_isr_wrapper(); // Declare external ISR wrapper

void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr);
void load_idt(void);
void initialize_idt(void);

#endif // IDT_H