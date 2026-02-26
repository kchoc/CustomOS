#ifndef x86_MSR_H
#define x86_MSR_H

// MSR - Model-Specific Register

#include <kern/types.h>

uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);

#endif // x86_MSR_H
