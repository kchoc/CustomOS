#include <kern/pcpu.h>
#include <inttypes.h>
#include <vm/layout.h>

#define LAPIC_REG(id)  ((volatile uint32_t*)(LAPIC_BASE + (id)))

#define LAPIC_ID       0x020
#define LAPIC_TPR      0x080
#define LAPIC_EOI      0x0B0
#define LAPIC_SVR      0x0F0
#define LAPIC_ICR_LOW  0x300
#define LAPIC_ICR_HIGH 0x310

#define LAPIC_TIMER_DIV  0x3E0
#define LAPIC_LVT_TIMER  0x320
#define TIMER_VECTOR    0x40
#define LAPIC_TIMER_INITCNT 0x380

#define ICR_DELIVERY_INIT  0x00000500U
#define ICR_DELIVERY_SIPI  0x00000600U
#define ICR_ASSERT_LEVEL    (1 << 14)   // assert level for INIT (some hardware)
#define ICR_EDGE_TRIGGER    0           // edge
#define ICR_PHYSICAL        0x00000000U // physical dest mode
#define ICR_FIXED           0x00000000U /* fixed delivery (for SIPI vector we still set SIPI) */

static inline uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t *addr = (volatile uint32_t*)(LAPIC_BASE + reg);
    return *addr;
}

inline void lapic_write(uint32_t reg, uint32_t value) {
    volatile uint32_t *addr = (volatile uint32_t*)(LAPIC_BASE + reg);
    *addr = value;
}

/* Read local APIC ID (bits 31:24 of ID register) */
uint32_t get_local_apic_id(void) {
    uint32_t val = lapic_read(LAPIC_ID);
    return (val >> 24) & 0xFF;
}

pcpu_t* get_current_pcpu(void) {
	uint32_t apic_id = get_local_apic_id();
	for (uint32_t i = 0; i < cpu_count; i++) {
		if (pcpus[i].apic_id == apic_id) {
			return &pcpus[i];
		}
	}
	return NULL;
}