#ifndef CPU_H
#define CPU_H

#include "kernel/descriptors/gdt.h"
#include "types/list.h"
#include "kernel/types.h"

#define MAX_CPUS 8

typedef struct thread thread_t;
typedef volatile uint32_t spinlock_t;

typedef struct cpu {
	uint32_t apic_id;    		// Local APIC ID
	thread_t *current_thread; 	// Currently running thread on this CPU
	list_t runqueue;
	uint32_t total_priority; 	// Total priority of all threads in the runqueue
	uint32_t cpu_number; 		// CPU number (0, 1, 2, ...)
	uint8_t started;    		// Has this CPU been started
	spinlock_t lock;			// Spinlock for synchronizing access to CPU data
	gdt_t gdt[GDT_ENTRIES]; 				// Per-CPU GDT
	gdt_ptr_t gdt_ptr;			// Per-CPU GDT pointer
	tss_t tss;					// Per-CPU TSS
} cpu_t;

extern cpu_t cpus[MAX_CPUS];
extern uint32_t cpu_count;

void init_cpus();

int map_apicid_to_index(uint32_t apic_id);
cpu_t* get_current_cpu();

#endif // CPU_H
