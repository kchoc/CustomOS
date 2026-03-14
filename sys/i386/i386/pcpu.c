#include <sys/pcpu.h>

#include <kern/terminal.h>

void machdep_init_pcpu(pcpu_t* pcpu, MACHDEP_PARAMS) {
  uint16_t sel = GSEL(GPRIV_SEL, SEL_KPL);
  asm volatile (
    "movw %0, %%fs\n" // Load selector into fs
    :
    : "r" (sel)
  );
  pcpu->apic_id = apic_id;
  write_tss(&pcpu->tss, GSEL(GDATA_SEL, SEL_KPL), 0);
}

pcpu_t* get_pcpu_by_apic_id(uint32_t apic_id) {
	for (uint32_t i = 0; i < cpu_count; i++) {
		if (pcpus[i].apic_id == apic_id) {
			return &pcpus[i];
		}
	}
	return NULL; // Not found
}

