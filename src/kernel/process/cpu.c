#include "kernel/process/cpu.h"
#include "kernel/process/lapic.h"
#include "kernel/process/ap_start.h"
#include "kernel/memory/kmalloc.h"
#include "types/string.h"

cpu_t cpus[MAX_CPUS] = {0};
uint32_t cpu_count = 0;

int map_apicid_to_index(uint32_t apic_id) {
	for (int i = 0; i < MAX_CPUS; i++) {
		if (cpus[i].apic_id == apic_id) {
			return i;
		}
	}
	return -1; // Not found
}

cpu_t* get_current_cpu() {
	uint32_t apic_id = get_local_apic_id();
	int index = map_apicid_to_index(apic_id);
	if (index < 0 || index >= cpu_count) {
		return NULL; // Invalid CPU
	}
	return &cpus[index];
}


void init_cpus() {
	for (uint32_t i = 0; i < cpu_count; i++) {
		cpus[i].cpu_number = i;
		cpus[i].runqueue.circular = 1;
		cpus[i].total_priority = 0;
		cpus[i].started = (i == 0) ? 1 : 0; // Bootstrap processor is started
		cpus[i].lock = 0;
	}

}

