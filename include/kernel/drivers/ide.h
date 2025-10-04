#ifndef IDE_H
#define IDE_H

#include "kernel/types.h"

void ide_write_sector(uint32_t lba, const uint8_t* data);
void ide_read_sector(uint32_t lba, uint8_t* buffer);

#endif // IDE_H