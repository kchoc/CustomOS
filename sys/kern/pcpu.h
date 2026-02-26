#ifndef PCPU_H
#define PCPU_H

#include <machine/pcpu.h>
#include "compiler.h"
#include "process.h"

#define MAX_CPUS 8

// Uses the GPRIV_SEL segment to get the current CPU structure, this is set up in i386/machdep.c when the GDT is initialized and each CPU is started
#define PCPU_GET(field) (get_current_cpu()->field)

static inline pcpu_t* get_current_cpu() {
	pcpu_t* pcpu;
	asm volatile("mov %%gs:0, %0" : "=r"(pcpu));
	return pcpu;
}

typedef struct pcpu {
	uint32_t pc_cpu_id; /* CPU identifier */
	list_t	runqueue;   /* Runqueue of threads for this CPU */
	thread_t* current_thread; /* Currently running thread on this CPU */
	uint32_t total_priority; /* Total priority of all threads in runqueue */
	uint8_t started; /* Has this CPU been started? */
	vm_space_t* vmspace; /* The kernel VM space, shared across all CPUs */
	PCPU_MD_FIELDS
} __packed pcpu_t;

extern pcpu_t pcpus[];
extern uint32_t cpu_count;

#endif // PCPU_H