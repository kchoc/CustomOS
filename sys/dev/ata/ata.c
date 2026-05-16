#include "ata.h"
#include "bus.h"
#include "types.h"

#include <disk/mbr.h>
#include <fs/vfs.h>
#include <sys/device.h>
#include <vm/kmalloc.h>

#include <dev/pci/pci.h>
#include <dev/port/port_io.h>

#include <kern/errno.h>
#include <kern/pit.h>
#include <kern/terminal.h>

#include <string.h>

DECLARE_DRIVER(ata_ide, ata);

/* ===========================
 * ATA Driver Implementation
 * =========================== */
int ata_ide_probe(device_t* dev)
{
    return 0; // This driver can handle any device, but we will only attach it to devices that we know are ATA drives in the bus enumeration code
}

int ata_ide_attach(device_t* dev)
{
    return 0; // All initialization is done in the bus enumeration code, so we don't need to do anything here
}

int ata_ide_detach(device_t* dev)
{
    return 0;
}

int ata_ide_suspend(device_t* dev)
{
    return 0;
}

int ata_ide_resume(device_t* dev)
{
    return 0;
}

int ata_ide_shutdown(device_t* dev)
{
    return 0;
}

/* ===========================
 * ATA Block Device Operations
 * =========================== */

int ata_ide_open(device_t* bdev)
{
    return 0;
}

int ata_ide_read(device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer)
{
    ata_drive_t* drive = (ata_drive_t*)bdev->softc;
    ata_wait_not_busy(drive);
    outb(drive->channel.io_base + ATA_REG_DEVICE, 0xE0 | (drive->channel.slave << 4) | ((lba >> 24) & 0x0F));
    ata_wait(drive->channel.control_base);

    outb(drive->channel.io_base + ATA_REG_SECTOR_COUNT, count);
    outb(drive->channel.io_base + ATA_REG_LBA_LOW, (uint8_t)lba);
    outb(drive->channel.io_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(drive->channel.io_base + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

    outb(drive->channel.io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS); // READ SECTORS
    ata_wait(drive->channel.control_base);

    // Wait for the drive to be ready
    for (uint32_t i = 0; i < count; i++) {
        if (ata_wait_drq(drive))
            return -EIO;

        insw(drive->channel.io_base + ATA_REG_DATA, buffer + i * 512, 256);
    }
    return 0;
}

int ata_ide_write(device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data)
{
    ata_drive_t* drive = (ata_drive_t*)bdev->softc;
    outb(drive->channel.io_base + ATA_REG_DEVICE, 0xE0 | (drive->channel.slave << 4) | ((lba >> 24) & 0x0F));
    ata_wait(drive->channel.control_base);

    outb(drive->channel.io_base + ATA_REG_SECTOR_COUNT, count);
    outb(drive->channel.io_base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(drive->channel.io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(drive->channel.io_base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);

    outb(drive->channel.io_base + ATA_REG_COMMAND, 0x30); // WRITE SECTORS

    // Wait for the drive to be ready to accept data
    int ret = ata_wait_drq(drive);
    if (ret)
        return ret;

    // Write the sector data
    outsw(drive->channel.io_base + ATA_REG_DATA, data, count * 256);

    // Wait for the write to complete
    return ata_wait_not_busy(drive);
}

int ata_ide_ioctl(device_t* bdev, int cmd, void* arg)
{
    switch (cmd) {
      case ATA_IOCTL_SOFTWARE_RESET:
        outb(((ata_drive_t*)bdev->softc)->channel.control_base + 0x02, 0x04); // Set SRST bit to initiate software reset
        delay_ms(5); // Wait 5ms for the drive to process the reset 
        outb(((ata_drive_t*)bdev->softc)->channel.control_base + 0x02, 0x00); // Clear SRST bit
        delay_ms(5); // Wait for the drive to reset
        return ata_wait_not_busy((ata_drive_t*)bdev->softc);
    default:
        return -EINVAL; // Unsupported command
    }
}

int ata_ide_close(device_t* bdev)
{
    return 0;
}

