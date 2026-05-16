#ifndef DEV_ATA_TYPES_H
#define DEV_ATA_TYPES_H

#include <kern/compiler.h>

#include <inttypes.h>

// ATA Register Offsets
#define ATA_REG_DATA          0x00
#define ATA_REG_ERROR         0x01
#define ATA_REG_FEATURES      0x01
#define ATA_REG_SECTOR_COUNT  0x02
#define ATA_REG_LBA_LOW       0x03
#define ATA_REG_LBA_MID       0x04
#define ATA_REG_LBA_HIGH      0x05
#define ATA_REG_DEVICE        0x06
#define ATA_REG_COMMAND       0x07
#define ATA_REG_STATUS        0x07

// Status Register (SR) bits
#define ATA_SR_ERR            0x01 // Error
#define ATA_SR_IDX            0x02 // Index, Legacy, always 0
#define ATA_SR_CORR           0x04 // Error Code Correction, Legacy, indicates corrected data
#define ATA_SR_DRQ            0x08 // Data request ready
#define ATA_SR_DSC            0x10 // Drive seek complete
#define ATA_SR_DF             0x20 // Drive write fault (hardware error)
#define ATA_SR_DRDY           0x40 // Drive ready
#define ATA_SR_BSY            0x80 // Busy

// Error Register (ER) bits
#define ATA_ER_AMNF           0x01 // Address mark not found
#define ATA_ER_TK0NF          0x02 // Track 0 not found
#define ATA_ER_ABRT           0x04 // Command aborted
#define ATA_ER_MCR            0x08 // Media change request
#define ATA_ER_IDNF           0x10 // ID not found
#define ATA_ER_MC             0x20 // Media changed
#define ATA_ER_UNC            0x40 // Uncorrectable data error
#define ATA_ER_BBK            0x80 // Bad block

// Device/Head Register (DHR) bits
#define ATA_DHR_LBA           0x01 // LBA28 mode bits 27:24
#define ATA_DHR_DEV           0x10 // Device bit (0 for master, 1 for slave)
#define ATA_DHR_ALWAYS_1      0x20 // This bit is always 1 for ATA devices
#define ATA_DHR_LBA48         0x40 // LBA48 mode bit
#define ATA_DHR_ALWAYS_1_2    0x80 // This bit is always 1 for ATA devices

// Device Control Register (DCR) bits
#define ATA_DCR_ALWAYS_0      0x01 // This bit is always 0 for ATA devices
#define ATA_DCR_IEN           0x02 // Interrupt enable
#define ATA_DCR_SRST          0x04 // Software reset


// ATA Commands
#define ATA_CMD_NOP                   0x00

#define ATA_CMD_CFA_REQ_EXT           0x03 // CFA request extented error code

#define ATA_CMD_SET_MANAGEMENT        0x06 // Set features
#define ATA_CMD_SET_MANAGEMENT_XL     0x07 // Set features subcommand
#define ATA_CMD_DEVICE_RESET          0x08 // Device reset

#define ATA_CMD_REQ_SENSE_EXT         0x0B // Request sense data extended

#define ATA_CMD_GET_PHYS_ELEM_STATUS  0x12 // Get physical element status

#define ATA_CMD_READ_SECTORS          0x20
#define ATA_CMD_READ_SECTORS_NO_RETRY 0x21
#define ATA_CMD_READ_LONG             0x22
#define ATA_CMD_READ_LONG_NO_RETRY    0x23
#define ATA_CMD_READ_SECTORS_EXT      0x24
#define ATA_CMD_READ_DMA_EXT          0x25
#define ATA_CMD_READ_DMA_QUEUED_EXT   0x26
#define ATA_CMD_READ_NATIVE_MAX_ADDR_EXT  0x27 // Read native max address extented

#define ATA_CMD_READ_MULTIPLE_EXT     0x29
#define ATA_CMD_READ_STREAM_DMA_EXT   0x2A
#define ATA_CMD_READ_STREAM_EXT       0x2B

#define ATA_CMD_READ_LOG_EXT          0x2F
#define ATA_CMD_WRITE_SECTORS         0x30
#define ATA_CMD_WRITE_SECTORS_NO_RETRY  0x31
#define ATA_CMD_WRITE_LONG            0x32
#define ATA_CMD_WRITE_LONG_NO_RETRY   0x33
#define ATA_CMD_WRITE_SECTORS_EXT     0x34
#define ATA_CMD_WRITE_DMA_EXT         0x35
#define ATA_CMD_WRITE_DMA_QUEUED_EXT  0x36
#define ATA_CMD_SET_MAX_ADDR_EXT      0x37 // Set max address extented
#define ATA_CMD_CFA_WRITE_SECTORS_NO_ERASE 0x38
#define ATA_CMD_WRITE_MULTIPLE_EXT    0x39
#define ATA_CMD_WRITE_STREAM_DMA_EXT  0x3A
#define ATA_CMD_WRITE_STREAM_EXT      0x3B
#define ATA_CMD_WRITE_VERIFY          0x3C
#define ATA_CMD_WRITE_DMA_FUA_EXT     0x3D
#define ATA_CMD_WRITE_DMA_QUEUED_FUA_EXT 0x3E
#define ATA_CMD_WRITE_LOG_EXT         0x3F
#define ATA_CMD_READ_VERIFY_SECTORS   0x40
#define ATA_CMD_READ_VERIFY_SECTORS_NO_RETRY 0x41
#define ATA_CMD_READ_VERIFY_SECTORS_EXT 0x42

#define ATA_CMD_ZERO_EXT              0x44
#define ATA_CMD_WRITE_UNCORRECTABLE_EXT 0x45

#define ATA_CMD_READ_LOG_DMA_EXT      0x47

#define ATA_CMD_ZAC_MANAGEMENT_IN     0x4A // Zone append command management in

#define ATA_CMD_FORMAT_TRACK          0x50
#define ATA_CMD_CONFIGURE_STREAM      0x51

#define ATA_CMD_WRITE_LONG_DMA_EXT    0x57

#define ATA_CMD_TRUSTED_NON_DATA      0x5B
#define ATA_CMD_TRUSTED_RECEIVE       0x5C
#define ATA_CMD_TRUSTED_RECEIVE_DMA   0x5D
#define ATA_CMD_TRUSTED_SEND          0x5E
#define ATA_CMD_TRUSTED_SEND_DMA      0x5F
#define ATA_CMD_READ_FPDMA_QUEUED     0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED    0x61

#define ATA_CMD_NCQ_NON_DATA          0x63
#define ATA_CMD_SEND_FPDMA_QUEUED     0x64
#define ATA_CMD_RECEIVE_FPDMA_QUEUED  0x65

#define ATA_CMD_SEEK                  0x70 // 0x70 -> 0x7F are various seek and execute device diagnostic commands
#define ATA_CMD_SET_DATE_AND_TIME_EXT 0x77
#define ATA_CMD_ACCESSIBLE_MAX_ADDR_CONF 0x78
#define ATA_CMD_REMOVE_ELEM_AND_TRUNCATE 0x7C
#define ATA_CMD_RESTORE_ELEM_AND_REBUILD 0x7D
#define ATA_CMD_REMOVE_ELEM_AND_MOD_ZONES 0x7E

#define ATA_CMD_VENDOR_SPECIFIC       0x80 // 0x80 -> 0x8F are vendor specific commands
#define ATA_CMD_CFA_TRANSLATE_SECTOR  0x87

#define ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define ATA_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define ATA_CMD_DOWNLOAD_MICROCODE    0x92
#define ATA_CMD_DOWNLOAD_MICROCODE_DMA 0x93
#define ATA_CMD_STANDBY_IMMEDIATE_OLD 0x94
#define ATA_CMD_IDLE_IMMEDIATE_OLD    0x95
#define ATA_CMD_MUTATE_EXT            0x96
#define ATA_CMD_STANDBY_OLD               0x96
#define ATA_CMD_IDLE_OLD                  0x97
#define ATA_CMD_CHECK_POWER_MODE_OLD      0x98
#define ATA_CMD_SLEEP_OLD                 0x99
#define ATA_CMD_VENDOR_SPECIFIC_1     0x9A

#define ATA_CMD_ZAC_MANAGEMENT_OUT    0x9F // Zone append command management out
#define ATA_CMD_PACKET                0xA0 // ATAPI packet command
#define ATA_CMD_IDENTIFY_PACKET_DEV   0xA1 // ATAPI identify packet device
#define ATA_CMD_SERVICE               0xA2 // Service command

#define ATA_CMD_SMART                 0xB0
#define ATA_CMD_DEV_CONFIG_OVERLAY    0xB1
#define ATA_CMD_SET_SECTOR_CONFIG_EXT 0xB2

#define ATA_CMD_SANATIZE_DEV          0xB4

#define ATA_CMD_NV_CACHE              0xB6
#define ATA_CMD_CFA_RESERVED          0xB7 // 0xB7 -> 0xBB are CFA reserved commands

#define ATA_CMD_CFA_ERASE_SECTORS     0xC0
#define ATA_CMD_VENDOR_SPECIFIC_2     0xC1 // 0xC1 -> 0xC3 are vendor specific commands

#define ATA_CMD_READ_MULTIPLE         0xC4
#define ATA_CMD_WRITE_MULTIPLE        0xC5
#define ATA_CMD_SET_MULTIPLE_MODE     0xC6
#define ATA_CMD_READ_DMA_QUEUED       0xC7
#define ATA_CMD_READ_DMA              0xC8
#define ATA_CMD_READ_DMA_NO_RETRY     0xC9
#define ATA_CMD_WRITE_DMA             0xCA
#define ATA_CMD_WRITE_DMA_NO_RETRY    0xCB
#define ATA_CMD_WRITE_DMA_QUEUED      0xCC
#define ATA_CMD_CFA_WRITE_MULTIPLE_NO_ERASE 0xCD
#define ATA_CMD_WRITE_MULTIPLE_FUA_EXT 0xCE

#define ATA_CMD_CHECK_MEDIA_CARD_TYPE 0xD1

#define ATA_CMD_GET_MEDIA_STATUS      0xDA
#define ATA_CMD_ACKNOWLEDGE_MEDIA_CHANGE 0xDB
#define ATA_CMD_POST_BOOT             0xDC
#define ATA_CMD_PRE_BOOT              0xDD
#define ATA_CMD_MEDIA_LOCK                 0xDE
#define ATA_CMD_MEDIA_UNLOCK               0xDF
#define ATA_CMD_STANDBY_IMMEDIATE     0xE0
#define ATA_CMD_IDLE_IMMEDIATE        0xE1
#define ATA_CMD_STANDBY              0xE2
#define ATA_CMD_IDLE                 0xE3
#define ATA_CMD_READ_BUFFER          0xE4
#define ATA_CMD_CHECK_POWER_MODE          0xE5
#define ATA_CMD_SLEEP                0xE6
#define ATA_CMD_FLUSH_CACHE           0xE7
#define ATA_CMD_WRITE_BUFFER         0xE8
#define ATA_CMD_READ_BUFFER_DMA     0xE9
#define ATA_CMD_WRITE_SAME            0xE9
#define ATA_CMD_FLUSH_CACHE_EXT           0xEA
#define ATA_CMD_WRITE_BUFFER_DMA   0xEB
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_MEDIA_EJECT          0xED
#define ATA_CMD_IDENTIFY_DEVICE_DMA 0xEE
#define ATA_CMD_SET_FEATURES              0xEF
#define ATA_CMD_VENDOR_SPECIFIC_3     0xF0 // 0xF0 -> 0xFF are vendor specific commands
#define ATA_CMD_SECURITY_SET_PASSWORD       0xF1
#define ATA_CMD_SECURITY_UNLOCK             0xF2
#define ATA_CMD_SECURITY_ERASE_PREPARE      0xF3
#define ATA_CMD_SECURITY_ERASE_UNIT         0xF4
#define ATA_CMD_SECURITY_FREEZE_LOCK        0xF5
#define ATA_CMD_SECURITY_DISABLE_PASSWORD    0xF6

#define ATA_CMD_READ_NATIVE_MAX_ADDR        0xF8
#define ATA_CMD_SET_MAX_ADDR               0xF9

/* --------------------
 * ATA Type Definitions
 * -------------------- */

typedef struct ata_identify_data {
    uint16_t config;
    uint16_t cylinders;
    uint16_t reserved1;
    uint16_t heads;
    uint16_t reserved2;
    uint16_t sectors_per_track;
    uint16_t reserved3[3];
    char     serial_number[20];
    uint16_t reserved4[3];
    char     firmware_revision[8];
    char     model_number[40];
    uint16_t reserved5[2];
    uint16_t capabilities;
    uint16_t reserved6[2];
    uint16_t valid_ext_data;
    uint16_t reserved7[5];
    uint64_t total_sectors;
} __packed ata_identify_data_t;

typedef struct ata_channel {
    uint16_t            io_base;
    uint16_t            control_base;
    uint16_t            bm_io_base;   // For bus mastering, if supported
    uint8_t             irq;          // IRQ number for this channel
} ata_channel_t;

typedef struct ide_channel_regs {
    uint16_t            io_base;
    uint16_t            control_base;
    uint16_t            slave; // 0 for master, 1 for slave
    uint8_t             IEN;   // Interrupt Enable
} ide_channel_regs_t;

typedef struct ata_drive {
    ide_channel_regs_t  channel;
    ata_identify_data_t identify_data;
} ata_drive_t;

#endif // DEV_ATA_TYPES_H
