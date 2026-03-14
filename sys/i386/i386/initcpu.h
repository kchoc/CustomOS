#ifndef I386_INITCPU_H
#define I386_INITCPU_H

#include <kern/pcpu.h>

int init_primary_cpu(pcpu_t* pcpu);
int init_secondary_cpu(pcpu_t* pcpu);

#endif // I386_INITCPU_H
