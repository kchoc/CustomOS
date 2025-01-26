#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// ISR handler definition
void isr_default_handler(void);
void isr_keyboard_handler(void);

// Exception handlers
void isr_divide_by_zero(void);
void isr_debug(void);
void isr_non_maskable_interrupt(void);
void isr_breakpoint(void);
void isr_overflow(void);
void isr_bound_range_exceeded(void);
void isr_invalid_opcode(void);
void isr_device_not_available(void);
void isr_double_fault(void);
void isr_invalid_tss(void);
void isr_segment_not_present(void);
void isr_stack_segment_fault(void);
void isr_general_protection_fault(void);
void isr_page_fault(void);
void isr_fpu_error(void);
void isr_alignment_check(void);
void isr_machine_check(void);
void isr_simd_floating_point(void);

#endif // ISR_H