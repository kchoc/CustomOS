#include "kernel/memory/vm.h"
#include "kernel/drivers/port_io.h"
#include "kernel/panic.h"
#include "kernel/terminal.h"

static uint8_t hasPanicOccurred = 0;

void panic(const char *message, const char *file, int line) {
    printf("KERNEL PANIC: %s\n", message);
    printf("File: %s, Line: %d\n", file, line);
    halt_system();
}

void panic_assert(const char *expression, const char *file, int line) {
    printf("ASSERTION FAILED: %s\n", expression);
    printf("File: %s, Line: %d\n", file, line);
    halt_system();
}

void panic_dump_registers(registers_t* regs) {
    printf("REGISTER DUMP:\n");
    printf("EAX: %x\n", regs->eax);
    printf("EBX: %x\n", regs->ebx);
    printf("ECX: %x\n", regs->ecx);
    printf("EDX: %x\n", regs->edx);
    printf("ESI: %x\n", regs->esi);
    printf("EDI: %x\n", regs->edi);
    printf("EBP: %x\n", regs->ebp);
    printf("ESP: %x\n", regs->esp);
    printf("EIP: %x (phys %x)\n", regs->eip, vmm_resolve((void*)regs->eip));
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
    outb(0x64, 0xFE);
    while (1) { }
}
