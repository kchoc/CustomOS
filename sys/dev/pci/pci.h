#ifndef DEV_PCI_H
#define DEV_PCI_H

#include <sys/bus.h>
#include <sys/device.h>

#include <inttypes.h>

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

// PCI device config read helpers
uint32_t pci_dev_config_read32(pci_device_t* dev, uint8_t offset);
uint16_t pci_dev_config_read16(pci_device_t* dev, uint8_t offset);
uint32_t pci_dev_config_write32(pci_device_t* dev, uint8_t offset, uint32_t value);

uint32_t pci_read_bar(pci_device_t* dev, uint8_t bar_index);

// Enumerate PCI devices and print their info
int pci_enumerate(bus_t* bus);
int pci_match(device_t* dev, driver_t* drv);
int pci_probe(device_t* dev, driver_t* drv);
int pci_remove(device_t* dev);

// Get the base address register (BAR) of a PCI device
uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar_index);

extern bus_t pci_bus;

#endif // DEV_PCI_H
