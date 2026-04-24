#include "device.h"

#include <dev/ata/ata.h>
#include <dev/vga/vga.h>

#include <disk/mbr.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

// For now im doing to use a static list of drivers
driver_t drivers[] = {{.name        = "PCI Mass Storage Driver",
                       .vendor_id   = 0x8086, // Intel
                       .device_id   = 0x7010, // PIIX4 IDE Controller
                       .device_type = DEV_TYPE_BLOCK,
                       .probe       = ata_pci_probe_controller},
                      {.name        = "PCI ATA Driver",
                       .vendor_id   = 0x8086, // Intel
                       .device_id   = 0x2922, // 82801DB/DBM SATA Controller
                       .device_type = DEV_TYPE_BLOCK,
                       .probe       = ata_pci_probe_controller},
                      {0},
                      {0},
                      {0}}; // Null entries to allow for more drivers to be added later

static int num_drivers = 2;

static int device_counter    = 0;
static int device_id_counter = 0;

int device_misc_create(driver_t* driver, device_ops_t* ops, device_t** out_dev)
{
    device_t* dev = kmalloc(sizeof(device_t));
    if (!dev)
        return -ENOMEM;

    snprintf(dev->name, MAX_DEVICE_NAME_LEN, "%s%d", driver->name, device_id_counter++);
    dev->type     = DEV_TYPE_GENERIC;
    dev->bus_type = 0; // No bus for misc devices
    dev->bus      = &bus_none;
    dev->ops      = ops;
    dev->driver   = driver;

    *out_dev = dev;
    return 0; // Success
}

int device_register(device_t* dev)
{
    for (int i = 0; i < num_drivers; ++i) {
        if (dev->bus->match(dev, &drivers[i])) {
            return dev->bus->probe(dev, &drivers[i]);
        }
    }
    return -1; // No matching driver found
}

int register_block_device(device_t* bdev)
{
    return mbr_parse(bdev);
}

int driver_register(driver_t* driver)
{
    // For simplicity, we just add the driver to our static list
    if (num_drivers >= sizeof(drivers) / sizeof(drivers[0])) {
        return -1; // No more space for drivers
    }
    drivers[num_drivers++] = *driver;
    return 0; // Success
}
