#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/terminal.h"
#include "kernel/memory/page.h"
#include "types/common.h"
#include "kernel/syscalls/syscalls.h"
#include <string.h>

IsrFunction g_interrupt_handlers[256];

void interrupt_register(uint8_t n, IsrFunction interrupt_handler) {
    g_interrupt_handlers[n] = interrupt_handler;
}

// The default handler for unhanded interrupts
void handle_isr(Registers regs) {
    uint8_t int_no = regs.interruptNumber & 0xFF;

    if (g_interrupt_handlers[int_no] != 0) {
        IsrFunction handler = g_interrupt_handlers[int_no];
        handler(&regs);
    }
    else
    {
        printf("unhandled interrupt: %d\n", int_no);
    }
}

// The keyboard ISR handler (IRQ1 -> Interrupt vector 33)
void isr_keyboard_handler(Registers *regs) {    
    // Read the scan code from the keyboard
    uint8_t scan_code = inb(0x60);

    // Print the scan code
    handle_keypress(scan_code);

    // Send an EOI to the PIC
    outb(0x20, 0x20); // Send EOI to PIC1
    outb(0xA0, 0x20); // Send EOI to PIC2
}

// The page fault ISR handler
void isr_page_fault_handler(Registers *regs) {
    uint32_t faulting_address;
    asm volatile("movl %%cr2, %0" : "=r" (faulting_address));

    printf("===== Page Fault =====\n");
    printf("Return Address: %x\n", &regs->eip);
    printf("Fault Address: %x\n", faulting_address);
    delay(300);


    // Map the page
    if (page_table_map(faulting_address, 0, PAGE_FLAG_ALLOCATE | PAGE_FLAG_READWRITE)) {
        printf("Failed to map page at %x\n", faulting_address);
        asm volatile("hlt");
    }

    page_table_refresh();

    // Update PIC
    outb(0x20, 0x20); // Send EOI to PIC1
    outb(0xA0, 0x20); // Send EOI to PIC2
}

void isr_syscall(Registers *regs) {

    if (regs->eax >= SYSCALL_COUNT) {
        printf("Unknown syscall!\n");
        regs->eax = -1;
        return;
    }

    void *syscall = g_syscalls[regs->eax];

    if (syscall == NULL) {
        printf("Syscall not defined!\n");
        regs->eax = -1;
        return;
    }

    // Push the parameters onto the stack. to call c syscall function. then pop them off the stack.
    int ret;
    asm volatile (" \
      pushl %1; \
      pushl %2; \
      pushl %3; \
      pushl %4; \
      pushl %5; \
      call *%6; \
      popl %%ebx; \
      popl %%ebx; \
      popl %%ebx; \
      popl %%ebx; \
      popl %%ebx; \
    " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (syscall));
    regs->eax = ret;
}

// Exception handlers
void isr_divide_by_zero(Registers *regs) {
    printf("Divide by zero\n");
    asm volatile("hlt");
}

void isr_debug(Registers *regs) {
    printf("Debug\n");
    asm volatile("hlt");
}

void isr_non_maskable_interrupt(Registers *regs) {
    printf("Non-maskable interrupt\n");
    asm volatile("hlt");
}

void isr_breakpoint(Registers *regs) {
    printf("Breakpoint\n");
    asm volatile("hlt");
}

void isr_overflow(Registers *regs) {
    printf("Overflow\n");
    asm volatile("hlt");
}

void isr_bound_range_exceeded(Registers *regs) {
    printf("Bound range exceeded\n");
    asm volatile("hlt");
}

void isr_invalid_opcode(Registers *regs) {
    printf("Invalid opcode\n");
    asm volatile("hlt");
}

void isr_device_not_available(Registers *regs) {
    printf("Device not available\n");
    asm volatile("hlt");
}

void isr_double_fault(Registers *regs) {
    printf("Double fault\n");
    asm volatile("hlt");
}

void isr_coprocessor_segment_overrun(Registers *regs) {
    printf("Coprocessor Segment Overrun\n");
    asm volatile("hlt");
}

void isr_invalid_tss(Registers *regs) {
    printf("Invalid TSS\n");
    asm volatile("hlt");
}

void isr_segment_not_present(Registers *regs) {
    printf("Segment not present\n");
    asm volatile("hlt");
}

void isr_stack_segment_fault(Registers *regs) {
    printf("Stack segment fault\n");
    asm volatile("hlt");
}

void isr_general_protection_fault(Registers *regs) {
    printf("General protection fault\n");
    asm volatile("hlt");
}

void isr_fpu_error(Registers *regs) {
    printf("FPU error\n");
    asm volatile("hlt");
}

void isr_alignment_check(Registers *regs) {
    printf("Alignment check\n");
    asm volatile("hlt");
}

void isr_machine_check(Registers *regs) {
    printf("Machine check\n");
    asm volatile("hlt");
}

void isr_simd_floating_point(Registers *regs) {
    printf("SIMD floating point\n");
    asm volatile("hlt");
}
