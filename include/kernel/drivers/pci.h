#ifndef PCI_H
#define PCI_H

#include "kernel/types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef struct pci_device {
	uint8_t bus;
	uint8_t slot;
	uint8_t function;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
} pci_device_t;

// Functions to read/write PCI configuration space
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);

void pci_discover_devices(void);

// Enumerate PCI devices and print their info
void pci_enumerate_devices(void);

// Get the base address register (BAR) of a PCI device
uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar_index);

#endif // PCI_H
