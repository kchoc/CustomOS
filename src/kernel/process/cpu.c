#include "kernel/process/cpu.h"
#include "kernel/process/lapic.h"
#include "kernel/process/ap_start.h"

#include "types/string.h"
#include <stdint.h>

int map_apicid_to_index(uint32_t apic_id) {
	for (int i = 0; i < MAX_CPUS; i++) {
		if (cpus[i].apic_id == apic_id) {
			return i;
		}
	}
	return -1; // Not found
}

void init_cpus() {
	for (uint32_t i = 0; i < MAX_CPUS; i++) {
		cpus[i].apic_id = 0;
		cpus[i].current_thread = NULL;
		cpus[i].runqueue_head = NULL;
		cpus[i].runqueue_tail = NULL;
		cpus[i].cpu_number = i;
		cpus[i].started = (i == 0) ? 1 : 0; // Boot CPU is started
		cpus[i].lock = 0; // Initialize spinlock to unlocked state
	}

	lapic_init(); // Initialize LAPIC

	// Discover APIC IDs of all CPUs
	uint32_t apic_ids[MAX_CPUS];
	uint32_t apic_count = 0;
	// discover_apic_ids(apic_ids, &apic_count);

	start_all_aps(apic_ids, apic_count); // Start all APs


}

