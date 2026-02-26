#ifndef IDE_H
#define IDE_H

#include <inttypes.h>

typedef struct ide_device {
    uint8_t  reserved;    // 0 (Empty) or 1 (This Drive really exists).
    uint8_t  channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
    uint8_t  drive;       // 0 (Master Drive) or 1 (Slave Drive).
    uint16_t type;        // 0: ATA, 1:ATAPI.
    uint16_t signature;   // Drive Signature
    uint16_t capabilities;// Features.
    uint32_t command_sets;// Command Sets Supported.
    uint32_t size;       // Size in Sectors.
    char     model[41];  // Model in string.
} ide_device_t;

typedef struct block_device {
    char*       name;
    uint32_t    sector_size;
    uint32_t    total_sectors;
    void        (*read_sector)( uint32_t lba, uint8_t* buffer);
    void        (*write_sector)(uint32_t lba, const uint8_t* data);
    ide_device_t* ide_dev;
} block_device_t;

void ide_read_sector( uint32_t lba, uint8_t* buffer);
void ide_write_sector(uint32_t lba, const uint8_t* data);
void ide_detect_all_drives();

#endif // IDE_H