#include "kernel/panic.h"
#include "kernel/terminal.h"



void panic(const char *message, const char *file, int line) {
	terminal_clear();
	printf("KERNEL PANIC: %s\n", message);
	printf("File: %s, Line: %d\n", file, line);
	halt_system();
}

void panic_assert(const char *expression, const char *file, int line) {
	terminal_clear();
	printf("ASSERTION FAILED: %s\n", expression);
	printf("File: %s, Line: %d\n", file, line);
	halt_system();
}

void panic_dump_registers(Registers* regs) {
	terminal_clear();
	printf("REGISTER DUMP:\n");
	printf("EAX: %x\n", regs->eax);
	printf("EBX: %x\n", regs->ebx);
	printf("ECX: %x\n", regs->ecx);
	printf("EDX: %x\n", regs->edx);
	printf("ESI: %x\n", regs->esi);
	printf("EDI: %x\n", regs->edi);
	printf("EBP: %x\n", regs->ebp);
	printf("ESP: %x\n", regs->esp);
	printf("EIP: %x\n", regs->eip);
	printf("EFLAGS: %x\n", regs->eflags);
	halt_system();
}

void halt_system(void) {
	hasPanicOccurred = 1;
	while (1) {
		__asm__ __volatile__("hlt");
	}
}

void reboot_system(void) {
	// Use keyboard controller to pulse CPU reset line
	__asm__ __volatile__(
		"cli\n"
		"mov $0x64, %dx\n"
		"in %dx, %al\n"
		"or $0x02, %al\n"
		"out %al, %dx\n"
		"jmp .\n"
	);
	while (1) { }
}
