#ifndef AP_START_H
#define AP_START_H

#include <stdint.h>

int start_ap(uint32_t apic_id);
void start_all_aps(uint32_t *apic_ids, uint32_t count);

void ap_entry_point(void);

#endif // AP_START_H