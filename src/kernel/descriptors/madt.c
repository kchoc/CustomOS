#include "kernel/descriptors/madt.h"
#include "kernel/descriptors/rsd.h"
#include "kernel/terminal.h"

void parse_madt(acpi_header_t *madt_header) {
    madt_t *madt = (madt_t *)madt_header;

    lapic_entry_t* entry = (lapic_entry_t*)madt->entries;
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    while ((uint8_t*)entry < end) {
        if (entry->type == 0 && entry->flags & 1) { // Local APIC and enabled
            printf("Found CPU with APIC ID: %u\n", entry->apic_id);
        }
        entry = (lapic_entry_t*)((uint8_t*)entry + entry->length);
    }
}