#ifndef SYS_DRIVER_H
#define SYS_DRIVER_H

#include "bus.h"

#include <list.h>
#include <inttypes.h>

typedef struct device device_t;

typedef struct device_lifecycle_ops {
    int (*probe)(device_t* dev);
    int (*attach)(device_t* dev);
    int (*detach)(device_t* dev);
    int (*suspend)(device_t* dev);
    int (*resume)(device_t* dev);
    int (*shutdown)(device_t* dev);
} dev_lc_ops_t;

typedef struct device_ops {
    int (*open) (device_t* dev);
    int (*read) (device_t* dev, uint64_t offset, uint32_t size, uint8_t* buffer);
    int (*write)(device_t* dev, uint64_t offset, uint32_t size, const uint8_t* data);
    int (*ioctl)(device_t* dev, int cmd, void* arg);
    int (*close)(device_t* dev);
} device_ops_t;

typedef struct driver {
    char          name[32];

    enum bus_type bus_type; // For driver declarations that specify a bus type
    list_node_t bus_node; // For drivers registered to a bus's driver list
 
    dev_lc_ops_t* lifecycle_ops;
    device_ops_t* ops;
    bus_ops_t* bus_ops;

    void* driver_data; // Data specific to the driver
} driver_t;

#define get_driver_from_node(node) container_of(node, driver_t, bus_node)

#define DECLARE_DRIVER_LC_OPS(drv_name)     \
    int drv_name##_probe(device_t* dev);    \
    int drv_name##_attach(device_t* dev);   \
    int drv_name##_detach(device_t* dev);   \
    int drv_name##_suspend(device_t* dev);  \
    int drv_name##_resume(device_t* dev);   \
    int drv_name##_shutdown(device_t* dev); \
    dev_lc_ops_t drv_name##_lc_ops = {      \
        .probe = drv_name##_probe,          \
        .attach = drv_name##_attach,        \
        .detach = drv_name##_detach,        \
        .suspend = drv_name##_suspend,      \
        .resume = drv_name##_resume,        \
        .shutdown = drv_name##_shutdown }; 
    

#define DECLARE_DEVICE_OPS(drv_name)        \
    int drv_name##_open(device_t* dev);     \
    int drv_name##_read(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buffer); \
    int drv_name##_write(device_t* dev, uint64_t offset, uint32_t size, const uint8_t* buffer); \
    int drv_name##_ioctl(device_t* dev, int cmd, void* arg); \
    int drv_name##_close(device_t* dev);    \
    device_ops_t drv_name##_dev_ops = {     \
        .open = drv_name##_open,            \
        .read = drv_name##_read,            \
        .write = drv_name##_write,          \
        .ioctl = drv_name##_ioctl,          \
        .close = drv_name##_close };

#define _DEFINE_DRIVER(drv_name, dev_ops, lc_ops, bs_ops, bus_name) \
    static driver_t __driver_##drv_name __attribute__((section(".drivers"), used)) = { \
        .name   = #drv_name,                \
        .bus_type = bus_type_##bus_name,    \
        .lifecycle_ops = lc_ops,            \
        .ops = dev_ops,                     \
        .bus_ops = bs_ops,                  \
        .driver_data = NULL,                \
        .bus_node = {0} };

#define DECLARE_DRIVER(drv_name, bus_name) \
    DECLARE_DRIVER_LC_OPS(drv_name) \
    DECLARE_DEVICE_OPS(drv_name) \
    _DEFINE_DRIVER(drv_name, &drv_name##_dev_ops, &drv_name##_lc_ops, NULL, bus_name)

#define DECLARE_BUS_DRIVER(drv_name, bus_name) \
    DECLARE_DRIVER_LC_OPS(drv_name) \
    DECLARE_BUS_OPS(drv_name) \
    _DEFINE_DRIVER(drv_name, NULL, &drv_name##_lc_ops, &drv_name##_bus_ops, bus_name)

#define DECLARE_BRIDGE_DRIVER(drv_name, bus_name) \
    DECLARE_DRIVER_LC_OPS(drv_name) \
    DECLARE_DEVICE_OPS(drv_name) \
    DECLARE_BUS_OPS(drv_name) \
    _DEFINE_DRIVER(drv_name, &drv_name##_dev_ops, &drv_name##_lc_ops, &drv_name##_bus_ops, bus_name) 

extern driver_t __drivers_start[];
extern driver_t __drivers_end[];

extern list_t driver_lists[];

void drivers_init();

void get_drivers_for_bus(bus_type_t bus_type, list_t* driver_list);

#endif // SYS_DRIVER_H
