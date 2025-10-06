#include "kernel/drivers/ata.h"
#include "kernel/drivers/port_io.h"
#include "kernel/filesystem/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/time/pit.h"
#include "types/string.h"
#include "kernel/terminal.h"

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_SECTOR_COUNT    0x02
#define ATA_REG_LBA_LOW         0x03
#define ATA_REG_LBA_MID         0x04
#define ATA_REG_LBA_HIGH        0x05
#define ATA_REG_DEVICE          0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          0x07
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_SR_BSY              0x80
#define ATA_SR_DRQ              0x08

int ata_identify(uint16_t io_base, uint8_t slave) {
    outb(io_base + ATA_REG_DEVICE, 0xA0 | (slave << 4));
    delay_ms(1);

    outb(io_base + ATA_REG_SECTOR_COUNT, 0);
    outb(io_base + ATA_REG_LBA_LOW, 0);
    outb(io_base + ATA_REG_LBA_MID, 0);
    outb(io_base + ATA_REG_LBA_HIGH, 0);
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    delay_ms(1);

    uint8_t status = inb(io_base + ATA_REG_STATUS);
    if (status == 0) return -1;

    while ((status & ATA_SR_BSY) && !(status & ATA_SR_DRQ))
        status = inb(io_base + ATA_REG_STATUS);

    if (!(status & ATA_SR_DRQ)) return -1; // not an ATA device

    uint8_t buffer[256];
    insw(io_base + ATA_REG_DATA, buffer, 128);

    uint32_t sectors = buffer[60] | (buffer[61] << 16);
    if (sectors == 0) return -1; // No sectors, invalid device

    char model[41];
    // Change endianess of model string and remove trailing spaces
    for (int i = 0; i < 40; i += 2) {
        model[i] = buffer[54 + i + 1];
        model[i + 1] = buffer[54 + i];
    }
    model[40] = '\0';
    strrstrip(model);

    block_device_t* bdev = kmalloc(sizeof(block_device_t));
    if (!bdev) return -1;

    bdev->name = strdup(model);
    if (!bdev->name) bdev->name = "ata_disk";
    bdev->sector_size = 512;
    bdev->total_sectors = (buffer[60] | (buffer[61] << 16));
    bdev->read_sector = ide_read_sector;
    bdev->write_sector = ide_write_sector;
    bdev->ide_dev = NULL;

    return vfs_register_block_device(bdev);
}