#ifndef _I386_TRAPFRAME_H_
#define _I386_TRAPFRAME_H_

#include <inttypes.h>

typedef struct trapframe {
    uint32_t gs, fs, es, ds; // Segment registers
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t trap_num; // Interrupt number
    uint32_t err_code; // Error code (if applicable)
    uint32_t eip, cs, eflags; // Pushed by the processor on interrupt
    uint32_t user_esp, ss; // Only pushed if transitioning from user to kernel mode
} trapframe_t;

#endif // _I386_TRAPFRAME_H_

