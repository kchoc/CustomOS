#include "kernel/drivers/pci.h"
#include "kernel/drivers/port_io.h"
#include "kernel/drivers/ide.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"

#define PCI_VENDOR_INTEL 0x8086
#define PCI_DEVICE_PIIX4 0x7010 // Intel 82371AB/EB/MB PIIX4 ISA Bridge

static inline void pci_write_config_address(uint32_t address) {
    outl(PCI_CONFIG_ADDRESS, address);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t address = (1U << 31) | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC);
    pci_write_config_address(address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t value = pci_config_read32(bus, slot, function, offset & 0xFC);
    return (value >> ((offset & 2) * 8)) & 0xFFFF;
}

uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar_index) {
    return pci_config_read32(bus, slot, function, 0x10 + (bar_index * 4));
}

void activate_pci_driver(pci_device_t* dev) {
    if (!dev) return;

    if (dev->vendor_id == PCI_VENDOR_INTEL && dev->device_id == PCI_DEVICE_PIIX4)
        ide_detect_all_drives();
}

void pci_discover_devices(void) {
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor_id = pci_config_read16(bus, slot, 0, 0x00);
            if (vendor_id == 0xFFFF) continue;

            // Do NOT mask off bit 7 before testing it
            uint8_t header_type = pci_config_read16(bus, slot, 0, 0x0E) & 0xFF;
            uint8_t max_func = (header_type & 0x80) ? 8 : 1;

            for (uint8_t func = 0; func < max_func; func++) {
                vendor_id = pci_config_read16(bus, slot, func, 0x00);
                if (vendor_id == 0xFFFF) continue;

                pci_device_t* dev = kmalloc(sizeof(pci_device_t));                

                dev->vendor_id = vendor_id;
                dev->device_id = pci_config_read16(bus, slot, func, 0x02);
                dev->class_code = pci_config_read16(bus, slot, func, 0x0A) >> 8;
                dev->subclass   = pci_config_read16(bus, slot, func, 0x0A) & 0xFF;
                dev->prog_if    = pci_config_read16(bus, slot, func, 0x09) & 0xFF;
                dev->bus = bus;
                dev->slot = slot;
                dev->function = func;

                activate_pci_driver(dev);
            }
        }
    }
}

void pci_enumerate_devices(void) {
    printf("Scanning PCI bus...\n");
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor_id = pci_config_read16(bus, slot, 0, 0x00);
            if (vendor_id == 0xFFFF) continue;

            // Do NOT mask off bit 7 before testing it
            uint8_t header_type = pci_config_read16(bus, slot, 0, 0x0E) & 0xFF;
            uint8_t max_func = (header_type & 0x80) ? 8 : 1;

            for (uint8_t func = 0; func < max_func; func++) {
                vendor_id = pci_config_read16(bus, slot, func, 0x00);
                if (vendor_id == 0xFFFF) continue;

                uint16_t device_id = pci_config_read16(bus, slot, func, 0x02);
                uint8_t class_code = pci_config_read16(bus, slot, func, 0x0A) >> 8;
                uint8_t subclass   = pci_config_read16(bus, slot, func, 0x0A) & 0xFF;

                printf("PCI Device: Bus %u, Slot %u, Func %u\n", bus, slot, func);
                printf("  Vendor: 0x%04X, Device: 0x%04X, Class: 0x%02X, Subclass: 0x%02X\n",
                       vendor_id, device_id, class_code, subclass);
            }
        }
    }
}


