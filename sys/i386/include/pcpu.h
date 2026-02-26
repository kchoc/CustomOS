#ifndef PCPU_I386_H
#define PCPU_I386_H

#include <machine/tss.h>
#include <machine/segment.h>

#define PCPU_MD_FIELDS \
	tss_t  tss;          /* Task State Segment for this CPU */ \
	seg_desc_t pc_tss_desc; /* GDT descriptor for the TSS */ \
	uint32_t apic_id;

typedef struct pcpu pcpu_t;

pcpu_t* get_current_pcpu(void);

#endif // PCPU_H