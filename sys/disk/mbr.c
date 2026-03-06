#include "mbr.h"

#include <fs/vfs.h>
#include <stdint.h>
#include <vm/kmalloc.h>

#include <dev/ata/ata.h>
#include <kern/pit.h>

#include <kern/terminal.h>
#include <kern/errno.h>

block_ops_t mbr_block_ops = {
	.read = partition_read,
	.write = partition_write
};

int mbr_parse(block_device_t* bdev) {
	if (!bdev || !bdev->ops) return -1;

	uint8_t sector[512];
	bdev->ops->read(bdev, 0, 1, sector);

	mbr_t* mbr = (mbr_t*)sector;

	if (mbr->signature != MBR_SIGNATURE) return -1; // Invalid MBR signature

	for (int i = 0; i < 4; i++) {
		mbr_partition_entry_t* entry = &mbr->partitions[i];

		if (entry->partition_type == 0) continue; // Unused entry

		block_device_t* part_bdev = kmalloc(sizeof(block_device_t));
		if (!part_bdev) return -ENOMEM;

		partition_t* part = kmalloc(sizeof(partition_t));
		if (!part) {
			kfree(part_bdev);
			return -ENOMEM;
		}

		part->start_lba = entry->starting_lba;
		part->sector_count = entry->sector_count;

		snprintf(part_bdev->name, sizeof(part_bdev->name), "%sp%d", bdev->name, i + 1);
		part_bdev->dev = bdev->dev;
		part_bdev->sector_size = bdev->sector_size;
		part_bdev->sector_count = entry->sector_count;
		part_bdev->ops = &mbr_block_ops;
		part_bdev->private = part;
		part_bdev->parent = bdev;

		// Register the partition block device with the VFS
		vfs_register_block_device(part_bdev);
	}

	return 0;
}

int partition_read(block_device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer) {
	partition_t* part = (partition_t*)bdev->private;
	if (!part) return -1;

	return bdev->parent->ops->read(bdev->parent, part->start_lba + lba, count, buffer);
}

int partition_write(block_device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data) {
	partition_t* part = (partition_t*)bdev->private;
	if (!part) return -1;

	return bdev->parent->ops->write(bdev->parent, part->start_lba + lba, count, data);
}

