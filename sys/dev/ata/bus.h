#ifndef DEV_ATA_BUS_H
#define DEV_ATA_BUS_H

#include "types.h"

#include <sys/bus.h>

int ata_wait_not_busy(ata_drive_t* drive);
int ata_wait_drq(ata_drive_t* drive);
void ata_wait(uint16_t control_base);

extern driver_t* driver_ata;

#endif // DEV_ATA_BUS_H

