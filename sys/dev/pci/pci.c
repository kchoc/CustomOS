#include "pci.h"

#include <dev/port/port_io.h>

#include <vm/kmalloc.h>
#include <kern/terminal.h>

#define CLASS_CODE_MASS_STORAGE_CONTROLLER 0x01

#define PCI_VENDOR_INTEL 0x8086
#define PCI_DEVICE_PIIX4 0x7010 // Intel 82371AB/EB/MB PIIX4 ISA Bridge

#define PCI_CLASS_DISPLAY_CONTROLLER 0x03

bus_t pci_bus = {
    .name = "PCI",
    .enumerate = pci_enumerate,
    .match = pci_match,
    .probe = pci_probe,
    .remove = pci_remove
};

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

uint32_t pci_dev_config_read32(pci_device_t* dev, uint8_t offset) {
    return pci_config_read32(dev->bus, dev->slot, dev->function, offset);
}

uint16_t pci_dev_config_read16(pci_device_t* dev, uint8_t offset) {
    return pci_config_read16(dev->bus, dev->slot, dev->function, offset);
}

uint32_t pci_dev_config_write32(pci_device_t* dev, uint8_t offset, uint32_t value) {
    uint32_t address = (1U << 31) | (dev->bus << 16) | (dev->slot << 11) | (dev->function << 8) | (offset & 0xFC);
    pci_write_config_address(address);
    outl(PCI_CONFIG_DATA, value);
    return value;
}

inline uint16_t pci_read_vendor(uint8_t bus, uint8_t slot, uint8_t function) {
    return pci_config_read16(bus, slot, function, 0x00);
}

inline uint16_t pci_read_device_id(uint8_t bus, uint8_t slot, uint8_t function) {
    return pci_config_read16(bus, slot, function, 0x02);
}

inline uint8_t pci_read_class(uint8_t bus, uint8_t slot, uint8_t function) {
    return (pci_config_read32(bus, slot, function, 0x08) >> 24) & 0xFF;
}

inline uint8_t pci_read_subclass(uint8_t bus, uint8_t slot, uint8_t function) {
    return (pci_config_read32(bus, slot, function, 0x08) >> 16) & 0xFF;
}

inline uint8_t pci_read_prog_if(uint8_t bus, uint8_t slot, uint8_t function) {
    return (pci_config_read32(bus, slot, function, 0x08) >> 8) & 0xFF;
}

uint32_t pci_read_bar(pci_device_t* dev, uint8_t bar_index) {
    return pci_dev_config_read32(dev, 0x10 + (bar_index * 4));
}

int pci_enumerate(struct bus* bus) {
    uint8_t bus_num = 0;
    for (int i = 0; i < 256; ++i, ++bus_num) {
        for (uint8_t dev = 0; dev < 32; ++dev) {
            for (uint8_t func = 0; func < 8; ++func) {
                uint16_t vendor = pci_read_vendor(bus_num, dev, func);
                if (vendor == 0xFFFF) continue;

                pci_device_t* pdev = kmalloc(sizeof(pci_device_t));
                pdev->bus = bus_num;
                pdev->function = func;
                pdev->vendor_id = vendor;
                pdev->device_id = pci_read_device_id(bus_num, dev, func);
                pdev->class_code = pci_read_class(bus_num, dev, func);
                pdev->subclass = pci_read_subclass(bus_num, dev, func);
                pdev->prog_if = pci_read_prog_if(bus_num, dev, func);

                // Wrap in device_t
                device_t* dev_obj = kmalloc(sizeof(device_t));
                if (!dev_obj) {
                    // printf("Failed to allocate device object for PCI device %04X:%04X\n", pdev->vendor_id, pdev->device_id);
                    kfree(pdev);
                    continue;
                }
                snprintf(dev_obj->name, sizeof(dev_obj->name), "pci:%x:%x", pdev->vendor_id, pdev->device_id);
                dev_obj->bus_type = BUS_TYPE_PCI;
                dev_obj->bus = bus;
                dev_obj->bus_data = pdev;
                dev_obj->type = DEV_TYPE_GENERIC;

                device_register(dev_obj);
            }
        }
    }
    return 0;
}

int pci_match(device_t* dev, driver_t* drv) {
    pci_device_t* pci_dev = (pci_device_t*)dev->bus_data;
    if (drv->vendor_id == pci_dev->vendor_id && drv->device_id == pci_dev->device_id) {
        return 1;
    }
    return 0;
}

int pci_probe(device_t* dev, driver_t* drv) {
    if (!pci_match(dev, drv)) return -1;
    return drv->probe(dev);
}

int pci_remove(device_t* dev) {
    return 0;
}
