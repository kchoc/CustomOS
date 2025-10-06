#ifndef DEVICE_H
#define DEVICE_H

#include "kernel/types.h"

typedef enum {
    DEV_TYPE_BLOCK,
    DEV_TYPE_CHAR,
    DEV_TYPE_NET,
    DEV_TYPE_OTHER
} device_type_t;

typedef struct device {
    const char*    name;
    device_type_t  dev_type;
    int            dev_id;
    void*          private;

    int (*read)( struct device* dev,       void* buf, size_t len, size_t offset);
    int (*write)(struct device* dev, const void* buf, size_t len, size_t offset);
    int (*ioctl)(struct device* dev, int cmd, void* arg);
} device_t;

#endif // DEVICE_H
