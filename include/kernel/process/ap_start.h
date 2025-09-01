#ifndef AP_START_H
#define AP_START_H

#include <stdint.h>

int start_ap(uint32_t apic_id);
void start_all_aps(void);

void ap_entry(void);
void ipi_ap_test(void);

#endif // AP_START_H