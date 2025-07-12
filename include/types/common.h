#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef struct Registers
{
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pusha
    uint32_t interruptNumber, errorCode;             //if applicable
    uint32_t eip, cs, eflags, userEsp, ss;           //pushed by the CPU
} Registers;

typedef void (*IsrFunction)(Registers*);

#endif //COMMON_H