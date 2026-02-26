#ifndef LAPIC_H
#define LAPIC_H

#include <vm/layout.h>
#include <inttypes.h>

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

void lapic_write(uint32_t reg, uint32_t value);
void lapic_init(void);
void apic_timer_init(uint32_t ticks);
void apic_cpu_init(void);

void send_init_ipi(uint32_t apic_id);
void send_startup_ipi(uint32_t apic_id, uint32_t trampoline_paddr);

#endif // LAPIC_H