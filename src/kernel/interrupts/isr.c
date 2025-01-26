#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/terminal.h"

// The default handler for unhanded interrupts
void isr_default_handler() {
    outb(0x20, 0x20);  // Send an EOI to the PIC
}

// The keyboard ISR handler (IRQ1 -> Interrupt vector 33)
void isr_keyboard_handler() {
    // Read the scan code from the keyboard
    uint8_t scan_code = inb(0x60);

    // Print the scan code
    handle_keypress(scan_code);

    // Send an EOI to the PIC
    outb(0x20, 0x20); // Send EOI to PIC1
    outb(0xA0, 0x20); // Send EOI to PIC2
}

// The page fault ISR handler
void isr_page_fault_handler() {
    printf("Page fault\n");
}

// Exception handlers
void isr_divide_by_zero() {
    printf("Divide by zero\n");
}

void isr_debug() {
    printf("Debug\n");
}

void isr_non_maskable_interrupt() {
    printf("Non-maskable interrupt\n");
}

void isr_breakpoint() {
    printf("Breakpoint\n");
}

void isr_overflow() {
    printf("Overflow\n");
}

void isr_bound_range_exceeded() {
    printf("Bound range exceeded\n");
}

void isr_invalid_opcode() {
    printf("Invalid opcode\n");
}

void isr_device_not_available() {
    printf("Device not available\n");
}

void isr_double_fault() {
    printf("Double fault\n");
}

void isr_invalid_tss() {
    printf("Invalid TSS\n");
}

void isr_segment_not_present() {
    printf("Segment not present\n");
}

void isr_stack_segment_fault() {
    printf("Stack segment fault\n");
}

void isr_general_protection_fault() {
    printf("General protection fault\n");
}

void isr_page_fault() {
    printf("Page fault\n");
}

void isr_fpu_error() {
    printf("FPU error\n");
}

void isr_alignment_check() {
    printf("Alignment check\n");
}

void isr_machine_check() {
    printf("Machine check\n");
}

void isr_simd_floating_point() {
    printf("SIMD floating point\n");
}
