#ifndef DEV_ATA_H
#define DEV_ATA_H

#include <sys/device.h>

#include <inttypes.h>

typedef struct ata_identify_data {
	uint16_t config;
	uint16_t cylinders;
	uint16_t reserved1;
	uint16_t heads;
	uint16_t reserved2;
	uint16_t sectors_per_track;
	uint16_t reserved3[3];
	char serial_number[20];
	uint16_t reserved4[3];
	char firmware_revision[8];
	char model_number[40];
	uint16_t reserved5[2];
	uint16_t capabilities;
	uint16_t reserved6[2];
	uint16_t valid_ext_data;
	uint16_t reserved7[5];
	uint64_t total_sectors;
} ata_identify_data_t;

typedef struct ata_drive {
	uint16_t io_base;
	uint16_t control_base;
	uint8_t slave; // 0 for master, 1 for slave
	ata_identify_data_t identify_data;
} ata_drive_t;

typedef struct ata_controller {
	device_t* dev;
	uint32_t io_base;
	uint32_t control_base;
	ata_drive_t devices[2]; // Master and slave
} ata_controller_t;

int ata_pci_probe_controller(device_t* dev);
int ata_detect_drive(ata_controller_t* dev, uint8_t slave);

int ata_identify(ata_drive_t* dev);
int ata_read(block_device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer);
int ata_write(block_device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data);

int ata_software_reset(ata_drive_t* dev);

#endif // DEV_ATA_H
