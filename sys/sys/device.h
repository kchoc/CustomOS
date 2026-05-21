#ifndef SYS_DEVICE_H
#define SYS_DEVICE_H

#include "bus.h"
#include "driver.h"

#include <inttypes.h>
#include <stddef.h>

#define MAX_DEVICE_NAME_LEN 32

struct device;
struct partition;

typedef enum {
    DEV_TYPE_GENERIC = 0,
    DEV_TYPE_BLOCK,
    DEV_TYPE_CHAR,
    DEV_TYPE_NET,
    DEV_TYPE_BUS,
    DEV_TYPE_OTHER
} device_type_t;

typedef enum {
    DEV_STATE_PROBED = 0,
    DEV_STATE_ATTACHED,
    DEV_STATE_SUSPENDED,
    DEV_STATE_REMOVED
} device_state_t;

typedef struct device {
    /* -------------
     * Identification
     * ------------- */
    char          name[32];
    uint32_t      unit; // For devices that can have multiple instances (e.g., disk0, disk1)
    device_type_t type;

    /* ----------------
     * Device hierarchy
     * ---------------- */
    list_t children; // For devices that can have child devices (e.g., a PCI controller with child
                     // devices)
    list_node_t child_node; // Node for the parent's children list

    /* -----------------
     * Device operations
     * ----------------- */
    struct device_ops* ops;

    /* -------------
     * Driver
     * ------------- */
    struct driver* driver;

    /* -------------
     * Runtime state
     * ------------- */

    device_state_t state;
    uint32_t       ref_count;
    uint32_t       flags;

    /* --------------
     * Device numbers
     * -------------- */
    uint32_t major;
    uint32_t minor;
    uint64_t dev_id;

    /* -------------
     * Resources
     * ------------- */
    list_t resources;

    /* ---------------
     * DMA mask
     * --------------- */
    uint64_t dma_mask;

    /* --------------------------------
     * Device-specific software context
     * -------------------------------- */
    void* softc;

    /* -------------
     * Bus info
     * ------------- */
    void* bus_data;
} device_t;

#define device_get_parent(dev) container_of((dev)->child_node.list, device_t, children)

typedef struct partition {
    uint64_t start_lba;
    uint64_t sector_count;
    uint32_t block_size;
} partition_t;

void drivers_init();

int device_misc_create(driver_t* driver, device_t** out_dev);
int device_register(device_t* dev, bus_type_t type);
int register_block_device(device_t* bdev);

int driver_register(driver_t* driver);

#endif // SYS_DEVICE_H
