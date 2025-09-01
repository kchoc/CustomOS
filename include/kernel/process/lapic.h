#ifndef LAPIC_H
#define LAPIC_H

#include "kernel/memory/layout.h"

#include <stdint.h>

#define LAPIC_REG(id)  ((volatile uint32_t*)(LAPIC_BASE + (id)))

#define LAPIC_ID       0x020
#define LAPIC_EOI      0x0B0
#define LAPIC_SVR      0x0F0
#define LAPIC_ICR_LOW  0x300
#define LAPIC_ICR_HIGH 0x310

#define ICR_DELIVERY_INIT  0x00000500U
#define ICR_DELIVERY_SIPI  0x00000600U
#define ICR_ASSERT_LEVEL    (1 << 14)   // assert level for INIT (some hardware)
#define ICR_EDGE_TRIGGER    0           // edge
#define ICR_PHYSICAL        0x00000000U // physical dest mode
#define ICR_FIXED           0x00000000U /* fixed delivery (for SIPI vector we still set SIPI) */

uint32_t get_local_apic_id(void);

void lapic_init(void);
void send_init_ipi(uint32_t apic_id);
void send_startup_ipi(uint32_t apic_id, uint32_t trampoline_paddr);

#endif // LAPIC_H