#include "panic.h"
#include "terminal.h"

#include <dev/port/port_io.h>

#include <sys/pcpu.h>

#include <vm/vm_map.h>

#include <machine/pmap.h>

#include <string.h>

static uint8_t hasPanicOccurred = 0;

void panic(const char* message, const char* file, int line)
{
    printf("KERNEL PANIC: %s\n", message);
    printf("File: %s, Line: %d\n", file, line);
    halt_system();
}

void panic_res(const char* message, int res, const char* file, int line)
{
    printf("KERNEL PANIC: %s (Error code: %d)\n", message, res);
    printf("File: %s, Line: %d\n", file, line);
    halt_system();
}

void panic_assert(const char* expression, const char* file, int line)
{
    printf("ASSERTION FAILED: %s\n", expression);
    printf("File: %s, Line: %d\n", file, line);
    halt_system();
}

void panic_dump_registers(registers_t* regs)
{
    terminal_print_registers(regs);
    halt_system();
}

void halt_system(void)
{
    hasPanicOccurred = 1;
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void reboot_system(void)
{
    outb(0x64, 0xFE);
    while (1) {
    }
}
