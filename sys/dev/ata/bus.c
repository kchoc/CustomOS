#include "bus.h"
#include "ata.h"
#include "types.h"

#include <dev/pci/pci.h>
#include <dev/port/port_io.h>

#include <sys/device.h>

#include <vm/kmalloc.h>

#include <kern/terminal.h>
#include <kern/errno.h>

DECLARE_BUS_DRIVER(ata, any);
driver_t* driver_ata = &__driver_ata;

int ata_wait_not_busy(ata_drive_t* drive)
{
    uint8_t status;
    while ((status = inb(drive->channel.io_base + ATA_REG_STATUS)) & ATA_SR_BSY)
        ;
    if (status & (ATA_SR_ERR | ATA_SR_DF))
        return -EIO; // Error occurred
    return 0;
}

int ata_wait_drq(ata_drive_t* drive)
{
    uint8_t status;

    while (1) {
        status = inb(drive->channel.io_base + ATA_REG_STATUS);

        if (status & ATA_SR_ERR)
            return -EIO;
        if (status & ATA_SR_DF)
            return -EIO;

        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
            return 0;
    }
}

void ata_wait(uint16_t control_base)
{
    // Wait 400ns for the drive to process the command
    inb(control_base);
    inb(control_base);
    inb(control_base);
    inb(control_base);
}

int ata_enumerate(device_t* bus) 
{
    ata_channel_t* channel = (ata_channel_t*)bus->softc;

    // Detect drives on this controller
    for (int slave = 0; slave < 2; ++slave) {
        outb(channel->io_base + ATA_REG_DEVICE, 0xA0 | (slave << 4)); // Select drive
        ata_wait(channel->control_base);

        outb(channel->io_base + ATA_REG_SECTOR_COUNT, 0);
        outb(channel->io_base + ATA_REG_LBA_LOW, 0);
        outb(channel->io_base + ATA_REG_LBA_MID, 0);
        outb(channel->io_base + ATA_REG_LBA_HIGH, 0);
        outb(channel->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
        ata_wait(channel->control_base);

        // Check if the drive responded at all
        uint8_t status = inb(channel->io_base + ATA_REG_STATUS);
        if (status == 0) continue; // No drive present, move on to the next one

        // Wait for the drive to respond
        while (status & ATA_SR_BSY) {
            status = inb(channel->io_base + ATA_REG_STATUS);
            if (status & ATA_SR_ERR) return -EIO; // If an error occurs while waiting, treat it as no drive present TODO: This could also indicate a drive that is present but has an error condition, so we might want to handle this differently in the future
            if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Drive is ready
        }

        ata_drive_t* drive  = kmalloc(sizeof(ata_drive_t));
        if (!drive) return -ENOMEM;
        drive->channel.io_base      = channel->io_base;
        drive->channel.control_base = channel->control_base;
        drive->channel.slave        = slave;
        drive->channel.IEN          = 0; // TODO: Handle interrupts in the future

        ata_identify_data_t* id_data = &drive->identify_data;
        if (ata_wait_drq(drive))
            return -EIO;

        uint8_t buffer[512];

        insw(drive->channel.io_base + ATA_REG_DATA, buffer, 256);
        memcpy(id_data, buffer, sizeof(ata_identify_data_t));

        // Check if the drive is ATA or ATAPI based on the capabilities field
        uint16_t* total_sectors_addr =
            (uint16_t*)id_data + offsetof(ata_identify_data_t, total_sectors);
        if (id_data->capabilities & 0x200) {
            id_data->total_sectors = ((uint64_t)total_sectors_addr[3] << 48) |
                                 ((uint64_t)total_sectors_addr[2] << 32) |
                                 ((uint64_t)total_sectors_addr[1] << 16) | total_sectors_addr[0];
        }
        else {
            id_data->total_sectors =
                ((uint64_t)id_data->cylinders * id_data->heads * id_data->sectors_per_track);
        }

        device_t* bdev = kmalloc(sizeof(device_t));
        if (!bdev) {
            kfree(drive);
            return -ENOMEM;
        }

        snprintf(bdev->name, sizeof(bdev->name), "ata%d", slave);
        bdev->type        = DEV_TYPE_BLOCK;
        bdev->softc       = drive;
        bdev->state       = DEV_STATE_PROBED;

        ata_add_child(bus, bdev);

        device_register(bdev, bus_type_ata);

        register_block_device(bdev);
    }

    return 0;
}

int ata_add_child(device_t* bus, device_t* child)
{
    child->ref_count = 1;
    child->bus_data = NULL;
    list_push_head(&bus->children, &child->child_node);
    return 0;
}

int ata_remove_child(device_t* bus, device_t* child)
{
    return 0;
}

resource_t* ata_alloc_resource(device_t* bus, device_t* dev, resource_type_t type, size_t size)
{
    return NULL;
}

int ata_free_resource(device_t* bus, device_t* dev, resource_t* res)
{
    return 0;
}

int ata_setup_irq(device_t* bus, device_t* dev, int irq)
{
    return 0;
}

int ata_teardown_irq(device_t* bus, device_t* dev, int irq)
{
    return 0;
}

int ata_probe(device_t* dev)
{
    return 0; 
}

int ata_attach(device_t* dev)
{
    return 0;
}

int ata_detach(device_t* dev)
{
    return 0;
}

int ata_suspend(device_t* dev)
{
    return 0;
}

int ata_resume(device_t* dev)
{
    return 0;
}

int ata_shutdown(device_t* dev)
{
    return 0;
}

