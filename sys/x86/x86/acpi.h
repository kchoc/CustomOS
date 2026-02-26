#ifndef X86_ACPI_H
#define X86_ACPI_H

#include <kern/types.h>

int x86_acpi_init();
uint32_t x86_acpi_get_cpu_apic_id();
uint32_t x86_acpi_get_cpu_count();

#endif // X86_ACPI_H