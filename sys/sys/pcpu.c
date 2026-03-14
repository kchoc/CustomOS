#include "pcpu.h"

#include <vm/vm_space.h>

#define MAX_CPUS 8

pcpu_t pcpus[MAX_CPUS] = {0};
uint32_t cpu_count = 0;

// Initialises the next PCPU
int pcpu_init(MACHDEP_PARAMS) {
  pcpu_t* pcpu = &pcpus[cpu_count++];
  pcpu->self = pcpu;
  list_init(&pcpu->runqueue, 1);
	pcpu->current_thread = NULL;
	pcpu->total_priority = 0;
	pcpu->started = 0;
	pcpu->vmspace = kernel_vm_space;
  machdep_init_pcpu(pcpu, MACHDEP_ARGUMENTS);
	return 0;
}
