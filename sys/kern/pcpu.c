#include "pcpu.h"

#include <vm/vm_space.h>

#define MAX_CPUS 8

pcpu_t pcpus[MAX_CPUS] = {0};
uint32_t cpu_count = MAX_CPUS;

int pcpu_init() {
	for (uint32_t i = 0; i < cpu_count; i++) {
		list_init(&pcpus[i].runqueue, 1);
		pcpus[i].current_thread = NULL;
		pcpus[i].total_priority = 0;
		pcpus[i].started = 0;
		pcpus[i].vmspace = kernel_vm_space;
	}
	return 0;
}
