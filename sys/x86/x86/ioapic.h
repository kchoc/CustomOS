#ifndef IOAPIC_H
#define IOAPIC_H

#include <inttypes.h>

#define IOAPIC_BASE 0xFEC00000

void ioapic_init();
void ioapic_check();
void ioapic_enable_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id);

#endif // IOAPIC_H
