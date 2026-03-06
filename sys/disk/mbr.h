#ifndef DISK_MBR_H
#define DISK_MBR_H

#include <sys/device.h>

#include <inttypes.h>

#define MBR_SIGNATURE 0xAA55

typedef struct mbr_partition_entry {
	uint8_t boot_indicator; // 0x80 = bootable, 0x00 = non-bootable
	uint8_t start_head;
	uint8_t start_sector; // bits 0-5: sector, bits 6-7: high bits of cylinder
	uint8_t start_cylinder;
	uint8_t partition_type;
	uint8_t end_head;
	uint8_t end_sector; // bits 0-5: sector, bits 6-7: high bits of cylinder
	uint8_t end_cylinder;
	uint32_t starting_lba;
	uint32_t sector_count;
} __attribute__((packed)) mbr_partition_entry_t;

typedef struct mbr {
	uint8_t bootloader[446];
	mbr_partition_entry_t partitions[4];
	uint16_t signature;
} __attribute__((packed)) mbr_t;

// Parses the MBR and validates the signature. Adds partition entries to the system's vfs partition table.
int mbr_parse(block_device_t* bdev);

int partition_read(block_device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer);
int partition_write(block_device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data);

#endif // DISK_MBR_H
