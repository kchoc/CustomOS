#ifndef ATA_H
#define ATA_H

#include "kernel/types.h"

int ata_identify(uint16_t io_base, uint8_t slave);

#endif // ATA_H
