#include "kernel/descriptors/idt.h"
#include "kernel/interrupts/isr.h"
#include "kernel/drivers/port_io.h"
#include <stdint.h>

// The IDT and IDT pointer
struct idt_entry_t idt[IDT_SIZE];
struct idt_ptr_t ip;

// Function to set an IDT entry
void set_idt_entry(int index, uint32_t base, uint16_t selector, uint8_t type_attr) {
	idt[index].offset_low = base & 0xFFFF;
	idt[index].selector = selector;
	idt[index].zero = 0;
	idt[index].type_attributes = type_attr;
	idt[index].offset_high = (base >> 16) & 0xFFFF;
}

void remap_pic() {
	outb(0x20, 0x11); // Master PIC command
	outb(0xA0, 0x11); // Slave PIC command
	outb(0x21, 0x20); // Data PIC offset
	outb(0xA1, 0x28); // Slave PIC offset
	outb(0x21, 0x04); // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(0xA1, 0x02); // Tell Slave PIC its cascade identity (0000 0010)
	outb(0x21, 0x01); // 8086/88 (MCS-80/85) mode
	outb(0xA1, 0x01); // 8086/88 (MCS-80/85) mode

	outb(0x21, 0x0);  // Mask all interrupts except IRQ1
	outb(0xA1, 0x0);  // Mask all interrupts
}

// Load the IDT into the CPU using the LIDT instruction
void load_idt() {
	ip.limit = (sizeof(struct idt_entry_t) * IDT_SIZE) - 1;
	ip.base = (uint32_t)&idt;

	asm volatile("lidt (%0)" : : "r"(&ip));
}

// Initialize the IDT with the keyboard ISR and default handlers
void idt_init() {
	// Set up the default handler for all interrupts (for unused vectors)
	for (int i = 0; i < IDT_SIZE; ++i) {
        	set_idt_entry(i, 0, 0x08, 0x8E);  // Interrupt gate, present
	}

	set_idt_entry( 0, (uint32_t)isr0 , 0x08, 0x8E);
    set_idt_entry( 1, (uint32_t)isr1 , 0x08, 0x8E);
    set_idt_entry( 2, (uint32_t)isr2 , 0x08, 0x8E);
    set_idt_entry( 3, (uint32_t)isr3 , 0x08, 0x8E);
    set_idt_entry( 4, (uint32_t)isr4 , 0x08, 0x8E);
    set_idt_entry( 5, (uint32_t)isr5 , 0x08, 0x8E);
    set_idt_entry( 6, (uint32_t)isr6 , 0x08, 0x8E);
    set_idt_entry( 7, (uint32_t)isr7 , 0x08, 0x8E);
    set_idt_entry( 8, (uint32_t)isr8 , 0x08, 0x8E);
    set_idt_entry( 9, (uint32_t)isr9 , 0x08, 0x8E);
    set_idt_entry(10, (uint32_t)isr10, 0x08, 0x8E);
    set_idt_entry(11, (uint32_t)isr11, 0x08, 0x8E);
    set_idt_entry(12, (uint32_t)isr12, 0x08, 0x8E);
    set_idt_entry(13, (uint32_t)isr13, 0x08, 0x8E);
    set_idt_entry(14, (uint32_t)isr14, 0x08, 0x8E);
    set_idt_entry(15, (uint32_t)isr15, 0x08, 0x8E);
    set_idt_entry(16, (uint32_t)isr16, 0x08, 0x8E);
    set_idt_entry(17, (uint32_t)isr17, 0x08, 0x8E);
    set_idt_entry(18, (uint32_t)isr18, 0x08, 0x8E);
    set_idt_entry(19, (uint32_t)isr19, 0x08, 0x8E);
    set_idt_entry(20, (uint32_t)isr20, 0x08, 0x8E);
    set_idt_entry(21, (uint32_t)isr21, 0x08, 0x8E);
    set_idt_entry(22, (uint32_t)isr22, 0x08, 0x8E);
    set_idt_entry(23, (uint32_t)isr23, 0x08, 0x8E);
    set_idt_entry(24, (uint32_t)isr24, 0x08, 0x8E);
    set_idt_entry(25, (uint32_t)isr25, 0x08, 0x8E);
    set_idt_entry(26, (uint32_t)isr26, 0x08, 0x8E);
    set_idt_entry(27, (uint32_t)isr27, 0x08, 0x8E);
    set_idt_entry(28, (uint32_t)isr28, 0x08, 0x8E);
    set_idt_entry(29, (uint32_t)isr29, 0x08, 0x8E);
    set_idt_entry(30, (uint32_t)isr30, 0x08, 0x8E);
    set_idt_entry(31, (uint32_t)isr31, 0x08, 0x8E);
    set_idt_entry(32, (uint32_t)isr32, 0x08, 0x8E);
    set_idt_entry(33, (uint32_t)isr33, 0x08, 0x8E);
    set_idt_entry(128,(uint32_t)isr128,0x08, 0x8E);


    interrupt_register( 0, isr_divide_by_zero);
    interrupt_register( 1, isr_debug);
    interrupt_register( 2, isr_non_maskable_interrupt);
    interrupt_register( 3, isr_breakpoint);
    interrupt_register( 4, isr_overflow);
    interrupt_register( 5, isr_bound_range_exceeded);
    interrupt_register( 6, isr_invalid_opcode);
    interrupt_register( 7, isr_device_not_available);
    interrupt_register( 8, isr_double_fault);
    interrupt_register( 9, isr_coprocessor_segment_overrun);
    interrupt_register(10, isr_invalid_tss);
    interrupt_register(11, isr_segment_not_present);
    interrupt_register(12, isr_stack_segment_fault);
    interrupt_register(13, isr_general_protection_fault);
    interrupt_register(14, isr_page_fault_handler);

    interrupt_register(16, isr_fpu_error);
    interrupt_register(17, isr_alignment_check);
    interrupt_register(18, isr_machine_check);
    interrupt_register(19, isr_simd_floating_point);


    interrupt_register(33, isr_keyboard_handler);

    interrupt_register(128,isr_syscall);

	// Load the IDT into the CPU
	load_idt();

	// Remap the PIC
	remap_pic();
}
