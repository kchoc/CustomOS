#ifndef COMMON_H
#define COMMON_H

#define __user
#define __kernel

#include "kernel/types.h"

typedef struct registers
{
    uint32_t gs, fs, es, ds;                         //pushed the segs last
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pusha
    uint32_t interruptNumber, errorCode;             //if applicable
    uint32_t eip, cs, eflags, userEsp, ss;           //pushed by the CPU
} registers_t;

typedef void (*IsrFunction)(registers_t*);

#endif //COMMON_H