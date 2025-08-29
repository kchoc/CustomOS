#include "kernel/descriptors/rsd.h"
#include "kernel/descriptors/madt.h"
#include "kernel/memory/layout.h"
#include "kernel/panic.h"
#include "types/string.h"
#include "kernel/terminal.h"

static uint8_t sum_bytes(const uint8_t* p, unsigned len) {
    uint8_t s = 0;
    while (len--) s += *p++;
    return s;
}

static int rsdp_checksum_ok(const rsdp_t* r) {
    // ACPI v1 checksum (first 20 bytes)
    if (sum_bytes((const uint8_t*)r, 20) != 0) return 0;
    return 1;
}

rsdp_t* find_rsdp() {
    // 1) EBDA
    uint16_t ebda_seg = *(volatile uint16_t*)0x0000040E; // requires identity mapping of low 1MB or mapping of this page too
    if (ebda_seg) {
        uint32_t ebda = ((uint32_t)ebda_seg) << 4;
        uint32_t ebda_off = ebda - BIOS_START;
        if (ebda >= BIOS_START && (ebda + 1024) <= (BIOS_START + BIOS_SIZE)) {
            for (uint32_t off = ebda_off; off < ebda_off + 1024; off += 16) {
                rsdp_t* r = (rsdp_t*)(BIOS_VIRT + off);
                if (strncmp(r->signature, "RSD PTR ", 8) == 0 && rsdp_checksum_ok(r)) return r;
            }
        }
    }

    // 2) BIOS area 0xE0000..0xFFFFF
    for (uint32_t off = 0; off < BIOS_SIZE; off += 16) {
        rsdp_t* r = (rsdp_t*)(BIOS_VIRT + off);
        if (strncmp(r->signature, "RSD PTR ", 8) == 0 && rsdp_checksum_ok(r)) return r;
    }
    return NULL;
}

acpi_header_t* find_table(rsdt_t* rsdt, const char* signature) {
	uint32_t entry_count = (rsdt->header.length - sizeof(acpi_header_t)) / sizeof(uint32_t);
	for (uint32_t i = 0; i < entry_count; i++) {
		acpi_header_t* header = (acpi_header_t*)(uintptr_t)rsdt->entries[i];
		if (strncmp(header->signature, signature, 4) == 0) {
			
			return header; // Table found
		}
	}

	return NULL; // Table not found
}

void rsdt_init() {
	rsdp_t* rsdp = find_rsdp();
	if (!rsdp) PANIC("No RSDP found!");

	printf("RSDT found at %x\n", rsdp->rsdt_address);	
	for (int i = 0; i < 8; i++)
    	terminal_putchar(rsdp->signature[i]);
	delay(300);

	rsdt_t* rsdt = (rsdt_t*)(uintptr_t)rsdp->rsdt_address;
	acpi_header_t* madt = find_table(rsdt, "APIC");
	if (!madt) PANIC("No MADT found!");

	parse_madt(madt);
}
