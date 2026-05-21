#include "controller.h"
#include "bus.h"
#include "types.h"

#include <dev/pci/pci.h>
#include <dev/port/port_io.h>

#include <sys/driver.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

DECLARE_DRIVER(ata_controller, pci);

int ata_legacy_channel_exists(ata_channel_t* ch)
{
    // Check by writing and reading back from the sector count register, which is a common method to
    // detect legacy ATA channels
    outb(ch->io_base + ATA_REG_SECTOR_COUNT, 0x55);
    ata_wait(ch->control_base);
    uint8_t value = inb(ch->io_base + ATA_REG_SECTOR_COUNT);
    if (value != 0x55)
        return 0; // If we can't read back the value we wrote, the channel likely doesn't exist

    // Check if the primary channel exists by trying to read the status register
    uint8_t status = inb(ch->io_base + ATA_REG_STATUS);
    return (status != 0xFF); // If we read 0xFF, it's likely that the channel doesn't exist
}

int ata_controller_probe(device_t* dev)
{
    pci_device_t* pci_dev = (pci_device_t*)dev->bus_data;
    if (pci_dev->vendor_id == 0x8086 &&
        (pci_dev->product_id == 0x7010 || pci_dev->product_id == 0x7111)) {
        return 0; // Found an Intel PIIX4 IDE controller
    }
    return -ENODEV; // Not a device we can handle
}

int ata_controller_attach(device_t* dev)
{
    snprintf(dev->name, sizeof(dev->name), "ata-pci-%04x:%04x",
             ((pci_device_t*)dev->bus_data)->vendor_id, ((pci_device_t*)dev->bus_data)->product_id);

    pci_device_t* pci_dev = (pci_device_t*)dev->bus_data;

    for (int i = 0; i < 2; ++i) {
        ata_channel_t* channel = kmalloc(sizeof(ata_channel_t));
        if (!channel)
            return -ENOMEM;

        if (pci_dev->prog_if & (1 << (i * 2))) {
            // TODO: Handle bus mastering if supported by the controller
            // PCI native mode, read I/O bases from BARs
            channel->io_base      = pci_read_bar(pci_dev, i * 2) & ~0x3;
            channel->control_base = pci_read_bar(pci_dev, i * 2 + 1) & ~0x3;
        }
        else {
            // Legacy IDE mode, use standard I/O ports
            if (i == 0) {
                channel->io_base      = 0x1F0;
                channel->control_base = 0x3F6;
                channel->irq          = 14; // Primary channel IRQ
            }
            else {
                channel->io_base      = 0x170;
                channel->control_base = 0x376;
                channel->irq          = 15; // Secondary channel IRQ
            }

            if (!ata_legacy_channel_exists(channel)) {
                kfree(channel);
                continue; // Skip this channel if it doesn't exist
            }
        }

        device_t* ata_bus = kmalloc(sizeof(device_t));
        if (!ata_bus) {
            kfree(channel);
            return -ENOMEM;
        }

        strncpy(ata_bus->name, (i == 0) ? "ata-primary" : "ata-secondary", sizeof(ata_bus->name));
        ata_bus->type   = DEV_TYPE_BUS;
        ata_bus->driver = driver_ata;
        list_init(&ata_bus->children, 0);
        ata_bus->bus_data  = NULL;
        ata_bus->ops       = NULL;
        ata_bus->state     = DEV_STATE_PROBED;
        ata_bus->ref_count = 1;
        ata_bus->softc     = channel;

        list_push_head(&dev->children, &ata_bus->child_node);

        ata_bus->driver->lifecycle_ops->attach(ata_bus);
        ata_bus->driver->bus_ops->enumerate(ata_bus);
    }

    return 0;
}

int ata_controller_detach(device_t* dev)
{
    return 0;
}

int ata_controller_suspend(device_t* dev)
{
    return 0;
}

int ata_controller_resume(device_t* dev)
{
    return 0;
}

int ata_controller_shutdown(device_t* dev)
{
    return 0;
}

int ata_controller_open(device_t* dev)
{
    return 0;
}

int ata_controller_read(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buffer)
{
    return -EINVAL; // Not a readable device
}

int ata_controller_write(device_t* dev, uint64_t offset, uint32_t size, const uint8_t* data)
{
    return -EINVAL; // Not a writable device
}

int ata_controller_ioctl(device_t* dev, int cmd, void* arg)
{
    return -EINVAL; // No ioctl commands supported
}

int ata_controller_close(device_t* dev)
{
    return 0;
}
