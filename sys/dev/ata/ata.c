#include "ata.h"
#include <dev/port/port_io.h>
#include <fs/vfs.h>
#include <stdint.h>
#include <vm/kmalloc.h>
#include <kern/pit.h>
#include <disk/mbr.h>
#include <dev/pci/pci.h>
#include <sys/device.h>

#include <kern/terminal.h>
#include <kern/errno.h>
#include <string.h>

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_SECTOR_COUNT    0x02
#define ATA_REG_LBA_LOW         0x03
#define ATA_REG_LBA_MID         0x04
#define ATA_REG_LBA_HIGH        0x05
#define ATA_REG_DEVICE          0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          0x07

#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30
#define ATA_CMD_IDENTIFY        0xEC

#define ATA_SR_ERR              0x01 // Error
#define ATA_SR_IDX              0x02 // Index (always 0)
#define ATA_SR_CORR             0x04 // Corrected data
#define ATA_SR_DRQ              0x08 // Data request ready
#define ATA_SR_DSC              0x10 // Drive seek complete
#define ATA_SR_DF               0x20 // Drive write fault (not ready)
#define ATA_SR_DRDY             0x40 // Drive ready
#define ATA_SR_BSY              0x80 // Busy

block_ops_t ata_block_ops = {
	.read = ata_read,
	.write = ata_write
};

static int ata_wait_not_busy(ata_drive_t* drive) {
	uint8_t status;
	while ((status = inb(drive->io_base + ATA_REG_STATUS)) & ATA_SR_BSY);
	if (status & (ATA_SR_ERR | ATA_SR_DF)) return -EIO; // Error occurred
	return 0;
}

static int ata_wait_drq(ata_drive_t* drive) {
    uint8_t status;

    while (1) {
        status = inb(drive->io_base + ATA_REG_STATUS);

        if (status & ATA_SR_ERR) return -EIO;
        if (status & ATA_SR_DF) return -EIO;

        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
            return 0;
    }
}

static void ata_wait(uint16_t control_base) {
	// Wait 400ns for the drive to process the command
	inb(control_base);
	inb(control_base);
	inb(control_base);
	inb(control_base);
}

int ata_pci_probe_controller(device_t *dev) {
	pci_device_t* pci_dev = (pci_device_t*)dev->bus_data;

	ata_controller_t* controller = kmalloc(sizeof(ata_controller_t));
	if (!controller) return -ENOMEM;
	if (!(pci_dev->prog_if & 0x01)) {
		// Legacy IDE mode, only primary channel is used
		controller->io_base = 0x1F0;
		controller->control_base = 0x3F6;
	} else {
		// PCI native mode, read from BARs
		controller->io_base = pci_read_bar(pci_dev, 0) & ~0x3;
		controller->control_base = pci_read_bar(pci_dev, 1) & ~0x3;
	}

	dev->driver_data = controller;

	// Detect drives on this controller
	for (int i = 0; i < 2; ++i) {
		if (!ata_detect_drive(controller, i)) continue;

		block_device_t* bdev = kmalloc(sizeof(block_device_t));
		snprintf(bdev->name, sizeof(bdev->name), "hd%c", 'a' + i);
		bdev->dev = dev;
		bdev->sector_size = 512;
		bdev->sector_count = controller->devices[i].identify_data.total_sectors;
		bdev->ops = &ata_block_ops;
		bdev->private = &controller->devices[i];
		bdev->parent = NULL;

		// Register the block device with the VFS
		register_block_device(bdev);		
	}

	return 0;
}

int ata_detect_drive(ata_controller_t *dev, uint8_t slave) {
	outb(dev->io_base + ATA_REG_DEVICE, 0xA0 | (slave << 4)); // Select drive
	ata_wait(dev->control_base);

	outb(dev->io_base + ATA_REG_SECTOR_COUNT, 0);
	outb(dev->io_base + ATA_REG_LBA_LOW, 0);
	outb(dev->io_base + ATA_REG_LBA_MID, 0);
	outb(dev->io_base + ATA_REG_LBA_HIGH, 0);
	outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_wait(dev->control_base);

	uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
    if (status == 0) return 0;

	// Wait for the drive to respond
	while (status & ATA_SR_BSY)
		status = inb(dev->io_base + ATA_REG_STATUS);

	// Check if it's an ATA or ATAPI drive
	if (status & ATA_SR_ERR) return 0; // Not an ATA drive

	uint8_t lba_mid = inb(dev->io_base + ATA_REG_LBA_MID);
	uint8_t lba_high = inb(dev->io_base + ATA_REG_LBA_HIGH);

	// If both LBA_MID and LBA_HIGH are zero, it's likely an ATA drive
	if (lba_mid || lba_high) return 0; // Not an ATA drive

	ata_drive_t* drive = &dev->devices[slave];
	drive->io_base = dev->io_base;
	drive->control_base = dev->control_base;
	drive->slave = slave;

	if (ata_identify(drive)) return 0; // Failed to identify drive

	return 1;
}

int ata_identify(ata_drive_t *dev) {
	ata_identify_data_t* id_data = &dev->identify_data;
	 if (ata_wait_drq(dev))
        return -EIO;

    uint8_t buffer[512];

	insw(dev->io_base + ATA_REG_DATA, buffer, 256);
	memcpy(id_data, buffer, sizeof(ata_identify_data_t));

	// Check if the drive is ATA or ATAPI based on the capabilities field
	uint16_t* total_sectors_addr = (uint16_t*)id_data + offsetof(ata_identify_data_t, total_sectors);
	if (id_data->capabilities & 0x200) {
		id_data->total_sectors = ((uint64_t)total_sectors_addr[3] << 48) |
		                          ((uint64_t)total_sectors_addr[2] << 32) |
		                          ((uint64_t)total_sectors_addr[1] << 16) |
		                          total_sectors_addr[0];
	} else {
		id_data->total_sectors = ((uint64_t)id_data->cylinders * id_data->heads * id_data->sectors_per_track);
	}

	return 0;
}

int ata_read(block_device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer) {
	ata_drive_t* drive = (ata_drive_t*)bdev->private;
	ata_wait_not_busy(drive);
	outb(drive->io_base + ATA_REG_DEVICE, 0xE0 | (drive->slave << 4) | ((lba >> 24) & 0x0F));
	ata_wait(drive->control_base);
	
	outb(drive->io_base + ATA_REG_SECTOR_COUNT, count);
	outb(drive->io_base + ATA_REG_LBA_LOW, (uint8_t)lba);
	outb(drive->io_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
	outb(drive->io_base + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

	outb(drive->io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS); // READ SECTORS
	ata_wait(drive->control_base);

	// Wait for the drive to be ready
	for (uint32_t i = 0; i < count; i++) {
	    if (ata_wait_drq(drive))
	        return -EIO;

	    insw(drive->io_base + ATA_REG_DATA,
	        buffer + i * 512,
	        256);
	}
	return 0;
}

int ata_write(block_device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data) {
	ata_drive_t* drive = (ata_drive_t*)bdev->private;
	outb(drive->io_base + ATA_REG_DEVICE, 0xE0 | (drive->slave << 4) | ((lba >> 24) & 0x0F));
	ata_wait(drive->control_base);

	outb(drive->io_base + ATA_REG_SECTOR_COUNT, count);
	outb(drive->io_base + ATA_REG_LBA_LOW, lba & 0xFF);
	outb(drive->io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
	outb(drive->io_base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);

	outb(drive->io_base + ATA_REG_COMMAND, 0x30); // WRITE SECTORS

	// Wait for the drive to be ready to accept data
	int ret = ata_wait_drq(drive);
	if (ret) return ret;


	// Write the sector data
	outsw(drive->io_base + ATA_REG_DATA, data, count * 256);

	// Wait for the write to complete
	return ata_wait_not_busy(drive);
}

int ata_software_reset(ata_drive_t* drive) {
	outb(drive->control_base + 0x02, 0x04); // Set SRST bit
	delay_ms(5); // Wait 5ms
	outb(drive->control_base + 0x02, 0x00); // Clear SRST bit
	delay_ms(5); // Wait for drive to reset

	return ata_wait_not_busy(drive);
}
