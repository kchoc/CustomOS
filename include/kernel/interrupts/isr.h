#ifndef ISR_H
#define ISR_H

#include "types/common.h"

extern IsrFunction g_interrupt_handlers[];

void interrupt_register(uint8_t n, IsrFunction interrupt_handler);

// ISR handler definition
void isr_default_handler(void);
void isr_keyboard_handler(Registers *regs);
void isr_page_fault_handler(Registers *regs);
void isr_syscall(Registers *regs);

// Exception handlers
void isr_divide_by_zero(Registers *regs);
void isr_debug(Registers *regs);
void isr_non_maskable_interrupt(Registers *regs);
void isr_breakpoint(Registers *regs);
void isr_overflow(Registers *regs);
void isr_bound_range_exceeded(Registers *regs);
void isr_invalid_opcode(Registers *regs);
void isr_device_not_available(Registers *regs);
void isr_double_fault(Registers *regs);
void isr_coprocessor_segment_overrun(Registers *regs);
void isr_invalid_tss(Registers *regs);
void isr_segment_not_present(Registers *regs);
void isr_stack_segment_fault(Registers *regs);
void isr_general_protection_fault(Registers *regs);
void isr_fpu_error(Registers *regs);
void isr_alignment_check(Registers *regs);
void isr_machine_check(Registers *regs);
void isr_simd_floating_point(Registers *regs);

#endif // ISR_H