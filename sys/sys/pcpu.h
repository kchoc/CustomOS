#ifndef PCPU_H
#define PCPU_H

#include <machine/pcpu.h>

#include <kern/process.h>
#include <kern/spinlock.h>

#include <stdint.h>
#include <list.h>

#define MAX_CPUS 8

typedef struct pcpu {
  pcpu_t* self; /* Pointer to self for easy access from assembly */
  spinlock_t scheduler_lock; /* Spinlock for synchronizing access to the scheduler */
	uint32_t pc_cpu_id; /* CPU identifier */
	list_t	runqueue;   /* Runqueue of threads for this CPU */
	thread_t* current_thread; /* Currently running thread on this CPU */
	uint32_t total_priority; /* Total priority of all threads in runqueue */
	uint8_t started; /* Has this CPU been started? */
	vm_space_t* vmspace; /* The kernel VM space, shared across all CPUs */
	PCPU_MD_FIELDS
} pcpu_t;

extern pcpu_t pcpus[];
extern uint32_t cpu_count;

int pcpu_init(MACHDEP_PARAMS);

#endif // PCPU_H
