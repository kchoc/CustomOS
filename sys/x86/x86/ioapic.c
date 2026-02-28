#include "ioapic.h"
#include "vm/vm_map.h"

#include <kern/terminal.h>
#include <kern/errno.h>
#include <kern/panic.h>

#include <vm/types.h>
#include <vm/vm.h>
#include <vm/layout.h>

#include <string.h>

static uint32_t* ioapic = 0;

static inline void ioapic_write(uint32_t reg, uint32_t value) {
	ioapic[0] = reg;
	ioapic[4] = value;
}

static inline uint32_t ioapic_read(uint32_t reg) {
	ioapic[0] = reg;
	return ioapic[4];
}

void ioapic_init() {
    ioapic = vm_map_device(IOAPIC_BASE, PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, VM_REG_F_NONE);
    if (IS_ERR(ioapic)) PANIC("Failed to map IOAPIC");
}

void ioapic_check() {
	uint32_t id = (ioapic_read(0x00) >> 24) & 0x0F;
	uint32_t version = ioapic_read(0x01) & 0xFF;
	uint32_t max_redir = ((ioapic_read(0x01) >> 16) & 0xFF) + 1;

	printf("IOAPIC ID: %u, Version: %u, Max Redirection Entries: %u\n", id, version, max_redir);
}

void ioapic_enable_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id) {
	uint32_t redir_index = 0x10 + (irq * 2);
	
	// Set the low dword: vector, delivery mode (fixed), destination mode (physical), delivery status (idle), polarity (active high), trigger mode (edge), mask (unmasked)
	uint32_t low = vector;
	
	// Set the high dword: destination field
	uint32_t high = lapic_id << 24;
	
	ioapic_write(redir_index, low);
	ioapic_write(redir_index + 1, high);
}
