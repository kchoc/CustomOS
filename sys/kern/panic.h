#ifndef PANIC_H
#define PANIC_H

#include <libkern/common.h>
#include <inttypes.h>

static uint8_t hasPanicOccurred;

// Panic function to handle critical errors
void panic(const char *message, const char *file, int line);
void panic_assert(const char *expression, const char *file, int line);
void panic_dump_registers(registers_t* regs);

#define PANIC(msg) panic(msg, __FILE__, __LINE__)
#define PANIC_ASSERT(expr) ((expr) ? (void)0 : panic_assert(#expr, __FILE__, __LINE__))
#define PANIC_DUMP_REGISTERS(regs) panic_dump_registers(regs)

// Halt the system
void halt_system(void);

// Reboot the system
void reboot_system(void);

#endif // PANIC_H
