#include "madt.h"
#include "rsd.h"

#include <kern/pcpu.h>
#include <kern/terminal.h>

void parse_madt(acpi_header_t *madt_header) {
    madt_t *madt = (madt_t *)madt_header;

    lapic_entry_t* entry = (lapic_entry_t*)madt->entries;
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    uint32_t count = 0;
    while ((uint8_t*)entry < end) {
        if (entry->type == 0 && entry->flags & 1) { // Local APIC and enabled
            if (count < MAX_CPUS) {
                pcpus[count].apic_id = entry->apic_id;
                pcpus[count].started = (count == 0) ? 1 : 0; // BSP is started
                count++;
            } else {
                printf("Warning: Maximum CPU count reached (%d)\n", MAX_CPUS);
            }

        }
        entry = (lapic_entry_t*)((uint8_t*)entry + entry->length);
    }
    cpu_count = count;
}