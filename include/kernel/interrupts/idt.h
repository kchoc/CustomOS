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
extern void isr_divide_by_zero_wrapper();
extern void isr_debug_wrapper();
extern void isr_non_maskable_interrupt_wrapper();
extern void isr_breakpoint_wrapper();
extern void isr_overflow_wrapper();
extern void isr_bound_range_exceeded_wrapper();
extern void isr_invalid_opcode_wrapper();
extern void isr_device_not_available_wrapper();
extern void isr_double_fault_wrapper();
extern void isr_invalid_tss_wrapper();
extern void isr_segment_not_present_wrapper();
extern void isr_stack_segment_fault_wrapper();
extern void isr_general_protection_fault_wrapper();
extern void isr_page_fault_wrapper();
extern void isr_fpu_error_wrapper();
extern void isr_alignment_check_wrapper();
extern void isr_machine_check_wrapper();
extern void isr_simd_floating_point_wrapper();

extern void isr_keyboard_wrapper();

void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr);
void load_idt(void);
void idt_init(void);

#endif // IDT_H