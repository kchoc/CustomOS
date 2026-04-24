#ifndef SYS_DEVICE_H
#define SYS_DEVICE_H

#include "bus.h"

#include <inttypes.h>
#include <stddef.h>

#define MAX_DEVICE_NAME_LEN 32

struct device;
struct driver;
struct block_ops;
struct partition;

typedef enum { DEV_TYPE_GENERIC = 0, DEV_TYPE_BLOCK, DEV_TYPE_CHAR, DEV_TYPE_NET } device_type_t;

typedef struct device {
    char          name[32];
    device_type_t type;

    bus_type_t bus_type;
    bus_t*     bus;
    void*      bus_data; // Data specific to the bus (e.g., PCI device info)

    struct device_ops* ops;
    void*
        ops_data; // Data specific to the device operations (e.g., partition info for block devices)

    struct driver* driver;
    void*          driver_data; // Data specific to the driver

    struct device* parent;
} device_t;

typedef struct driver {
    char          name[32];
    uint16_t      vendor_id;
    uint16_t      device_id;
    device_type_t device_type;
    int (*probe)(device_t* dev);
} driver_t;

typedef struct device_ops {
    int (*read)(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buffer);
    int (*write)(device_t* dev, uint64_t offset, uint32_t size, const uint8_t* data);
} device_ops_t;

typedef struct partition {
    uint64_t start_lba;
    uint64_t sector_count;
    uint32_t block_size;
} partition_t;

#define DECLARE_DEVICE_TYPE(name)                                                                  \
    extern device_ops_t name##_ops;                                                                \
    extern driver_t     name##_driver;                                                             \
    int                 name##_probe(device_t* dev);                                               \
    int                 name##_read(device_t*, uint64_t, uint32_t, uint8_t*);                      \
    int                 name##_write(device_t*, uint64_t, uint32_t, const uint8_t*);

int device_misc_create(driver_t* driver, device_ops_t* ops, device_t** out_dev);
int device_register(device_t* dev);
int register_block_device(device_t* bdev);

int driver_register(driver_t* driver);

#endif // SYS_DEVICE_H
