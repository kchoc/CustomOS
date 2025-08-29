#ifndef RSD_H
#define RSD_H

#include <stdint.h>

typedef struct root_system_description_pointer {
	char signature[8]; // "RSD PTR "
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
} __attribute__((packed)) rsdp_t;

typedef struct advanced_configuration_and_power_interface_header {
	char signature[4]; // "APIC"
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;



typedef struct root_system_description_table {
	acpi_header_t header;
	uint32_t entries[]; // Array of pointers to other ACPI tables
} __attribute__((packed)) rsdt_t;

void rsdt_init();
rsdp_t* find_rsdp();
acpi_header_t* find_table(rsdt_t* rsdt, const char* signature);

#endif // RSD_H
