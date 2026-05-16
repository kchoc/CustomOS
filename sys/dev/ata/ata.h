#ifndef DEV_ATA_H
#define DEV_ATA_H

#include <sys/device.h>

#include <inttypes.h>

#define ATA_IOCTL_SOFTWARE_RESET 0x01

int ata_pci_probe_controller(device_t* dev);
int ata_detect_drive(device_t* cdev, uint8_t slave);

int ata_read(device_t* bdev, uint64_t lba, uint32_t count, uint8_t* buffer);
int ata_write(device_t* bdev, uint64_t lba, uint32_t count, const uint8_t* data);

#endif // DEV_ATA_H
