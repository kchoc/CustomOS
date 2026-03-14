#ifndef _I386_PCPU_H_
#define _I386_PCPU_H_

#include <machine/tss.h>
#include <machine/segment.h>

#define PCPU_MD_FIELDS \
	tss_t  tss;          /* Task State Segment for this CPU */ \
	seg_desc_t pc_tss_desc; /* GDT descriptor for the TSS */ \
	uint32_t apic_id; \
  uint32_t acpi_id;

#define MACHDEP_PARAMS uint32_t apic_id
#define MACHDEP_ARGUMENTS apic_id

typedef struct pcpu pcpu_t;

inline pcpu_t* get_pcpu(void) {
    struct pcpu *pc;
    // Assume pc_prvspace is the first field (offset 0)
    asm volatile ("movl %%fs:0, %0"
                  : "=r" (pc));
    return pc;
}

#define PCPU_GET(name)   (get_pcpu()->name)
#define PCPU_SET(name,v) (get_pcpu()->name = (v))

void machdep_init_pcpu(pcpu_t* pcpu, MACHDEP_PARAMS);
pcpu_t* get_pcpu_by_apic_id(uint32_t apic_id);

#endif // _I386_PCPU_H_
