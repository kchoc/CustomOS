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

// Declare external ISR wrappers
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr32();
extern void isr33();
extern void isr64();
extern void isr128();

void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr);
void load_idt(void);
void idt_init(void);

#endif // IDT_H