#include "device.h"

#include <dev/ata/ata.h>
#include <dev/vga/vga.h>

#include <disk/mbr.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

uint32_t device_id_counter = 0;

int device_misc_create(driver_t* driver, device_t** out_dev)
{
    device_t* dev = kmalloc(sizeof(device_t));
    if (!dev)
        return -ENOMEM;

    snprintf(dev->name, MAX_DEVICE_NAME_LEN, "%s%d", driver->name, device_id_counter++);
    dev->type      = DEV_TYPE_GENERIC;
    dev->state     = DEV_STATE_PROBED;
    dev->ref_count = 1;
    dev->driver    = driver;
    dev->ops       = driver->ops;

    driver->lifecycle_ops->attach(dev);

    *out_dev = dev;
    return 0; // Success
}

int device_register(device_t* dev, bus_type_t type)
{
    list_t*      driver_list = &driver_lists[type];
    list_node_t* node;
    list_for_each(node, driver_list)
    {
        driver_t* driver = get_driver_from_node(node);
        if (driver->lifecycle_ops->probe(dev) == 0) {
            // Found a matching driver
            dev->state  = DEV_STATE_PROBED;
            dev->driver = driver;
            dev->ops    = driver->ops;
            driver->lifecycle_ops->attach(dev);
            dev->state = DEV_STATE_ATTACHED;
            return 0; // Success
        }
    }
    return -1; // No matching driver found
}

int register_block_device(device_t* bdev)
{
    return mbr_parse(bdev);
}
