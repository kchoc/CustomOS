#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/panic.h"
#include "kernel/memory/vm.h"
#include "kernel/syscalls/syscalls.h"

#include "kernel/process/process.h"
#include "kernel/process/lapic.h"
#include "kernel/process/cpu.h"

#include "kernel/terminal.h"
#include "types/common.h"
#include "types/string.h"

IsrFunction g_interrupt_handlers[256];

void interrupt_register(uint8_t n, IsrFunction interrupt_handler) {
    g_interrupt_handlers[n] = interrupt_handler;
}

// The default handler for unhanded interrupts
void handle_isr(registers_t regs) {
    uint8_t int_no = regs.interruptNumber & 0xFF;

    if (g_interrupt_handlers[int_no] != 0) {
        IsrFunction handler = g_interrupt_handlers[int_no];
        handler(&regs);
    } else {
        PANIC("Unhandled interrupt");
    }
}

// The keyboard ISR handler (IRQ1 -> Interrupt vector 33)
void isr_keyboard_handler(registers_t *regs) {    
    // Read the scan code from the keyboard
    uint8_t scan_code = inb(0x60);

    // Print the scan code
    handle_keypress(scan_code);

    // Send an EOI to the LAPIC
    lapic_write(LAPIC_EOI, 0);
}

// The page fault ISR handler
void isr_page_fault_handler(registers_t *regs) {
    uint32_t* faulting_address;
    asm volatile("movl %%cr2, %0" : "=r" (faulting_address));

    printf("===== Page Fault =====\n");
    printf("CPU ID: %d\n", get_current_cpu()->apic_id);
    printf("CR3: %x\n", get_current_page_directory_phys());
    printf("Return Address: %x\n", &regs->eip);
    printf("Fault Address: %x\n", faulting_address);
    printf("Error Code: %x\n", regs->errorCode);
    delay(1000);

    // Map the page
    if (!vmm_map(faulting_address, 0, PAGE_SIZE, VM_PROT_READWRITE, VM_MAP_ZERO)) {
        printf("Failed to map page at %x\n", faulting_address);
        // PANIC_DUMP_REGISTERS(regs);
    }

    // Send an EOI to the LAPIC
    lapic_write(LAPIC_EOI, 0);
}

void isr_syscall(registers_t *regs) {
    if (regs->eax >= SYSCALL_COUNT) {
        printf("Invalid syscall number: %d\n", regs->eax);
        regs->eax = -1;
        return;
    }

    if (regs->eax == SYSCALL_EXIT) {
        syscall_exit(regs);
    }

    void *syscall = g_syscalls[regs->eax];

    if (syscall == NULL) {
        printf("Unimplemented syscall number: %d\n", regs->eax);
        regs->eax = -1;
        return;
    }

    // Push the parameters onto the stack. to call c syscall function. then pop them off the stack.
    syscall_fn_t syscall_fn = (syscall_fn_t)syscall;
    int ret = syscall_fn(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
    regs->eax = ret;
}

// Exception handlers
void isr_divide_by_zero(registers_t *regs) {
    printf("Divide by zero\n");
    asm volatile("hlt");
}

void isr_debug(registers_t *regs) {
    printf("Debug\n");
    asm volatile("hlt");
}

void isr_non_maskable_interrupt(registers_t *regs) {
    printf("Non-maskable interrupt\n");
    asm volatile("hlt");
}

void isr_breakpoint(registers_t *regs) {
    printf("Breakpoint\n");
    asm volatile("hlt");
}

void isr_overflow(registers_t *regs) {
    printf("Overflow\n");
    asm volatile("hlt");
}

void isr_bound_range_exceeded(registers_t *regs) {
    printf("Bound range exceeded\n");
    asm volatile("hlt");
}

void isr_invalid_opcode(registers_t *regs) {
    printf("Invalid opcode\n");
    PANIC_DUMP_REGISTERS(regs);
}

void isr_device_not_available(registers_t *regs) {
    printf("Device not available\n");
    asm volatile("hlt");
}

void isr_double_fault(registers_t *regs) {
    printf("Double fault\n");
    asm volatile("hlt");
}

void isr_coprocessor_segment_overrun(registers_t *regs) {
    printf("Coprocessor Segment Overrun\n");
    asm volatile("hlt");
}

void isr_invalid_tss(registers_t *regs) {
    printf("Invalid TSS\n");
    asm volatile("hlt");
}

void isr_segment_not_present(registers_t *regs) {
    printf("Segment not present\n");
    asm volatile("hlt");
}

void isr_stack_segment_fault(registers_t *regs) {
    printf("Stack segment fault\n");
    asm volatile("hlt");
}

void isr_general_protection_fault(registers_t *regs) {
    printf("General protection fault\n");
    printf("Error Code: %x\n", regs->errorCode);
    PANIC_DUMP_REGISTERS(regs);
}

void isr_fpu_error(registers_t *regs) {
    printf("FPU error\n");
    asm volatile("hlt");
}

void isr_alignment_check(registers_t *regs) {
    printf("Alignment check\n");
    asm volatile("hlt");
}

void isr_machine_check(registers_t *regs) {
    printf("Machine check\n");
    asm volatile("hlt");
}

void isr_simd_floating_point(registers_t *regs) {
    printf("SIMD floating point\n");
    asm volatile("hlt");
}

void isr_timer_handler(registers_t *regs) {
    // Send an EOI to the PIC
    outb(0x20, 0x20); // Send EOI to PIC1
    outb(0xA0, 0x20); // Send EOI to PIC2


    // Schedule next task
    schedule_from_irq(regs);

    // Send EOI to LAPIC
    lapic_write(LAPIC_EOI, 0);
}
