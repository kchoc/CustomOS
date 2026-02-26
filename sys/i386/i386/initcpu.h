#ifndef INITCPU_H
#define INITCPU_H

#include <kern/pcpu.h>

int init_primary_cpu(pcpu_t* pcpu);
int init_secondary_cpu(pcpu_t* pcpu);


#endif // INITCPU_H
