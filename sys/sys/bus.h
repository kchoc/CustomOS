#ifndef SYS_BUS_H
#define SYS_BUS_H

#include "resource.h"

#include <list.h>

typedef enum bus_type {
    bus_type_any = 0,
    bus_type_root,
    bus_type_pci,
    bus_type_ata,
    bus_type_usb,
    bus_type_virtio,
} bus_type_t;

typedef struct device device_t;
typedef struct driver driver_t;

typedef struct bus_ops {
    int (*enumerate)(device_t* bus);
    int (*add_child)(device_t* bus, device_t* child);
    int (*remove_child)(device_t* bus, device_t* child);

    resource_t* (*alloc_resource)(device_t* bus, device_t* dev, resource_type_t type, size_t size);
    int (*free_resource)(device_t* bus, device_t* dev, resource_t* res);

    int (*setup_irq)(device_t* bus, device_t* dev, int irq);
    int (*teardown_irq)(device_t* bus, device_t* dev, int irq);
} bus_ops_t;

#define DECLARE_BUS_OPS(name)                                                                      \
    int         name##_enumerate(device_t* bus);                                                   \
    int         name##_add_child(device_t* bus, device_t* child);                                  \
    int         name##_remove_child(device_t* bus, device_t* child);                               \
    resource_t* name##_alloc_resource(device_t* bus, device_t* dev, resource_type_t type,          \
                                      size_t size);                                                \
    int         name##_free_resource(device_t* bus, device_t* dev, resource_t* res);               \
    int         name##_setup_irq(device_t* bus, device_t* dev, int irq);                           \
    int         name##_teardown_irq(device_t* bus, device_t* dev, int irq);                        \
    bus_ops_t   name##_bus_ops = {.enumerate      = name##_enumerate,                              \
                                  .add_child      = name##_add_child,                              \
                                  .remove_child   = name##_remove_child,                           \
                                  .alloc_resource = name##_alloc_resource,                         \
                                  .free_resource  = name##_free_resource,                          \
                                  .setup_irq      = name##_setup_irq,                              \
                                  .teardown_irq   = name##_teardown_irq};

#endif // SYS_BUS_H
