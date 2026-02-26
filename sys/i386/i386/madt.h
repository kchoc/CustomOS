#ifndef MADT_H
#define MADT_H

#include "rsd.h"

#include <inttypes.h>

typedef struct multiple_apic_description_table {
    acpi_header_t header;
    uint32_t lapic_address;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed)) madt_t;

typedef struct local_apic_entry {
    uint8_t type;
    uint8_t length;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) lapic_entry_t;

void parse_madt(acpi_header_t *madt_header);

#endif // MADT_H
