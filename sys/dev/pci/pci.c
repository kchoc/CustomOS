#include "pci.h"

#include <dev/port/port_io.h>

#include <vm/kmalloc.h>

#include <kern/terminal.h>

DECLARE_BUS_DRIVER(pci, root);

static inline void pci_write_config_address(uint32_t address)
{
    outl(PCI_CONFIG_ADDRESS, address);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
    uint32_t address = (1U << 31) | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC);
    pci_write_config_address(address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
    uint32_t value = pci_config_read32(bus, slot, function, offset & 0xFC);
    return (value >> ((offset & 2) * 8)) & 0xFFFF;
}

uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar_index)
{
    return pci_config_read32(bus, slot, function, 0x10 + (bar_index * 4));
}

uint32_t pci_dev_config_read32(pci_device_t* dev, uint8_t offset)
{
    return pci_config_read32(dev->bus, dev->slot, dev->function, offset);
}

uint16_t pci_dev_config_read16(pci_device_t* dev, uint8_t offset)
{
    return pci_config_read16(dev->bus, dev->slot, dev->function, offset);
}

uint32_t pci_dev_config_write32(pci_device_t* dev, uint8_t offset, uint32_t value)
{
    uint32_t address =
        (1U << 31) | (dev->bus << 16) | (dev->slot << 11) | (dev->function << 8) | (offset & 0xFC);
    pci_write_config_address(address);
    outl(PCI_CONFIG_DATA, value);
    return value;
}

inline uint16_t pci_read_vendor(uint8_t bus, uint8_t slot, uint8_t function)
{
    return pci_config_read16(bus, slot, function, 0x00);
}

inline uint16_t pci_read_device_id(uint8_t bus, uint8_t slot, uint8_t function)
{
    return pci_config_read16(bus, slot, function, 0x02);
}

inline uint8_t pci_read_class(uint8_t bus, uint8_t slot, uint8_t function)
{
    return (pci_config_read32(bus, slot, function, 0x08) >> 24) & 0xFF;
}

inline uint8_t pci_read_subclass(uint8_t bus, uint8_t slot, uint8_t function)
{
    return (pci_config_read32(bus, slot, function, 0x08) >> 16) & 0xFF;
}

inline uint8_t pci_read_prog_if(uint8_t bus, uint8_t slot, uint8_t function)
{
    return (pci_config_read32(bus, slot, function, 0x08) >> 8) & 0xFF;
}

uint32_t pci_read_bar(pci_device_t* dev, uint8_t bar_index)
{
    return pci_dev_config_read32(dev, 0x10 + (bar_index * 4));
}

device_t* pci_detect(device_t* parent)
{
    // For simplicity, we assume PCI is always present
    device_t* pci_bus = kmalloc(sizeof(device_t));
    if (!pci_bus)
        return NULL;
    strncpy(pci_bus->name, "pci_bus", sizeof(pci_bus->name));
    pci_bus->type     = DEV_TYPE_BUS;
    pci_bus->bus_data = NULL;
    pci_bus->driver   = &__driver_pci;
    list_init(&pci_bus->children, 0);
    pci_bus->bus_data = NULL;

    list_push_head(&parent->children, &pci_bus->child_node);

    return pci_bus;
}

int pci_enumerate(device_t* bus)
{
    uint8_t bus_num = 0;
    for (int i = 0; i < 256; ++i, ++bus_num) {
        for (uint8_t dev = 0; dev < 32; ++dev) {
            for (uint8_t func = 0; func < 8; ++func) {
                uint16_t vendor = pci_read_vendor(bus_num, dev, func);
                if (vendor == 0xFFFF)
                    continue;

                pci_device_t* pdev = kmalloc(sizeof(pci_device_t));
                pdev->bus          = bus_num;
                pdev->function     = func;
                pdev->vendor_id    = vendor;
                pdev->product_id   = pci_read_device_id(bus_num, dev, func);
                pdev->class_code   = pci_read_class(bus_num, dev, func);
                pdev->subclass     = pci_read_subclass(bus_num, dev, func);
                pdev->prog_if      = pci_read_prog_if(bus_num, dev, func);

                // Wrap in device_t
                device_t* dev_obj = kmalloc(sizeof(device_t));
                if (!dev_obj) {
                    kfree(pdev);
                    continue;
                }
                snprintf(dev_obj->name, sizeof(dev_obj->name), "pci:%x:%x", pdev->vendor_id,
                         pdev->product_id);
                dev_obj->bus_data = pdev;
                dev_obj->type     = DEV_TYPE_GENERIC;

                list_push_head(&bus->children, &dev_obj->child_node);

                device_register(dev_obj, bus_type_pci);
            }
        }
    }
    return 0;
}

int pci_add_child(device_t* bus, device_t* child)
{
    list_push_head(&bus->children, &child->child_node);
    return 0;
}

int pci_remove_child(device_t* bus, device_t* child)
{
    list_remove(&child->child_node);
    return 0;
}

resource_t* pci_alloc_resource(device_t* bus, device_t* dev, resource_type_t type, size_t size)
{
    // For simplicity, we won't implement resource allocation in this example
    return NULL;
}

int pci_free_resource(device_t* bus, device_t* dev, resource_t* res)
{
    // For simplicity, we won't implement resource freeing in this example
    return 0;
}

int pci_setup_irq(device_t* bus, device_t* dev, int irq)
{
    // For simplicity, we won't implement IRQ setup in this example
    return 0;
}

int pci_teardown_irq(device_t* bus, device_t* dev, int irq)
{
    // For simplicity, we won't implement IRQ teardown in this example
    return 0;
}

int pci_probe(device_t* dev)
{
    // For simplicity, we won't implement probing in this example
    return 0;
}

int pci_attach(device_t* dev)
{
    // For simplicity, we won't implement attaching in this example
    return 0;
}

int pci_detach(device_t* dev)
{
    // For simplicity, we won't implement detaching in this example
    return 0;
}

int pci_suspend(device_t* dev)
{
    // For simplicity, we won't implement suspending in this example
    return 0;
}

int pci_resume(device_t* dev)
{
    // For simplicity, we won't implement resuming in this example
    return 0;
}

int pci_shutdown(device_t* dev)
{
    // For simplicity, we won't implement shutdown in this example
    return 0;
}
