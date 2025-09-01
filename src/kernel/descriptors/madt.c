#include "kernel/descriptors/madt.h"
#include "kernel/descriptors/rsd.h"
#include "kernel/process/cpu.h"
#include "kernel/terminal.h"

void parse_madt(acpi_header_t *madt_header) {
    madt_t *madt = (madt_t *)madt_header;

    lapic_entry_t* entry = (lapic_entry_t*)madt->entries;
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    while ((uint8_t*)entry < end) {
        if (entry->type == 0 && entry->flags & 1) { // Local APIC and enabled
            if (cpu_count < MAX_CPUS) {
                cpus[cpu_count].apic_id = entry->apic_id;
                cpus[cpu_count].started = (cpu_count == 0) ? 1 : 0; // BSP is started
                cpu_count++;
            } else {
                printf("Warning: Maximum CPU count reached (%d)\n", MAX_CPUS);
            }

        }
        entry = (lapic_entry_t*)((uint8_t*)entry + entry->length);
    }
}