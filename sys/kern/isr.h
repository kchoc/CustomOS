#ifndef ISR_H
#define ISR_H

#include <inttypes.h>
#include <libkern/common.h>

extern IsrFunction g_interrupt_handlers[];

void interrupt_register(uint8_t n, IsrFunction interrupt_handler);

// ISR handler definition
void isr_default_handler(void);
void isr_keyboard_handler(registers_t *regs);
void isr_page_fault_handler(registers_t *regs);
void isr_syscall(registers_t *regs);

// Exception handlers
void isr_divide_by_zero(registers_t *regs);
void isr_debug(registers_t *regs);
void isr_non_maskable_interrupt(registers_t *regs);
void isr_breakpoint(registers_t *regs);
void isr_overflow(registers_t *regs);
void isr_bound_range_exceeded(registers_t *regs);
void isr_invalid_opcode(registers_t *regs);
void isr_device_not_available(registers_t *regs);
void isr_double_fault(registers_t *regs);
void isr_coprocessor_segment_overrun(registers_t *regs);
void isr_invalid_tss(registers_t *regs);
void isr_segment_not_present(registers_t *regs);
void isr_stack_segment_fault(registers_t *regs);
void isr_general_protection_fault(registers_t *regs);
void isr_fpu_error(registers_t *regs);
void isr_alignment_check(registers_t *regs);
void isr_machine_check(registers_t *regs);
void isr_simd_floating_point(registers_t *regs);
void isr_timer_handler(registers_t *regs);

#endif // ISR_H