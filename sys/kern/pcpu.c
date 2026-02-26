#include "pcpu.h"

#define MAX_CPUS 8

pcpu_t pcpus[MAX_CPUS] = {0};
uint32_t cpu_count = MAX_CPUS;
