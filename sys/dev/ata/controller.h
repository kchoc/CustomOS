#ifndef DEV_ATA_CONTROLLER_H
#define DEV_ATA_CONTROLLER_H

#include <sys/device.h>

#include <inttypes.h>

typedef struct ata_controller {
    uint32_t    io_base;
    uint32_t    control_base;
} ata_controller_t;

#endif // DEV_ATA_CONTROLLER_H

