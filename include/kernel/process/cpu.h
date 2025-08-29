#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MAX_CPUS 8

typedef struct thread thread_t;
typedef volatile uint32_t spinlock_t;

typedef struct cpu {
	uint32_t apic_id;    // Local APIC ID
	thread_t *current_thread; // Currently running thread on this CPU
	thread_t *runqueue_head; // Head of the runqueue
	thread_t *runqueue_tail; // Tail of the runqueue
	uint32_t cpu_number; // CPU number (0, 1, 2, ...)
	uint8_t started;    // Has this CPU been started
	spinlock_t lock; // Spinlock for synchronizing access to CPU data
} cpu_t;

static cpu_t cpus[MAX_CPUS];
static uint32_t cpu_count = 1; // Boot CPU is always present

void init_cpus();

int map_apicid_to_index(uint32_t apic_id);

#endif // CPU_H
