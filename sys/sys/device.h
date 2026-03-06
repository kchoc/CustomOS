#ifndef SYS_DEVICE_H
#define SYS_DEVICE_H

#include "bus.h"

#include <inttypes.h>
#include <stddef.h>

struct device;
struct driver;
struct block_ops;
struct block_device;
struct partition;

typedef enum {
    DEV_TYPE_GENERIC = 0,
    DEV_TYPE_BLOCK,
    DEV_TYPE_CHAR,
    DEV_TYPE_NET
} device_type_t;

typedef struct device {
    char name[32];
    device_type_t type;

    bus_type_t bus_type;
    bus_t* bus;
    void* bus_data; // Data specific to the bus (e.g., PCI device info)

    struct driver* driver;
    void* driver_data; // Data specific to the driver

    struct device* parent;
} device_t;

typedef struct driver {
    char name[32];
    uint16_t vendor_id;
    uint16_t device_id;
    device_type_t device_type;
    int (*probe)(device_t* dev);
} driver_t;

typedef struct block_ops {
    int (*read)(struct block_device* bdev, uint64_t lba, uint32_t count, uint8_t* buffer);
    int (*write)(struct block_device* bdev, uint64_t lba, uint32_t count, const uint8_t* data);
} block_ops_t;

typedef struct block_device {
    char name[32];
	device_t* dev;

	uint64_t sector_count;
	uint32_t sector_size;

    const block_ops_t* ops;

    void* private; // partition-specific data
    struct block_device* parent; // For partitions, points to the parent block device
} block_device_t;

typedef struct partition {
    uint64_t start_lba;
    uint64_t sector_count;
} partition_t;

int device_register(device_t* dev);
int register_block_device(block_device_t* bdev);

#endif // SYS_DEVICE_H
