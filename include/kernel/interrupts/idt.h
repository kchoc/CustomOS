#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Define the number of entries in the IDT
#define IDT_SIZE 256

// IDT entry structure
typedef struct {
    uint16_t base_low;
    uint16_t base_high;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr);
void load_idt(void);
void initialize_idt(void);

extern void keyboard_isr_handler(void);
extern void isr_default_handler(void);

#endif // IDT_H