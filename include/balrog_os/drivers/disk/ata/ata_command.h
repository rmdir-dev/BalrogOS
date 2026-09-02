#pragma once

#include <stdint.h>

#define ATA_CMD_NOP                 0x00    // NOP no operation
#define ATA_CMD_CFA_RSV             0x03    // CFA reserved
#define ATA_CMD_DT_SET_MAN          0x06    // Data set management
#define ATA_CMD_DEV_RESET           0x08    // ATAPI soft reset / device reset.
#define ATA_CMD_REQ_SENS_DT_EXT     0x0b    // request sense data ext
#define ATA_CMD_RECAL_0             0x10
#define ATA_CMD_RECAL_1             0x11
#define ATA_CMD_RECAL_2             0x12
#define ATA_CMD_RECAL_3             0x13
#define ATA_CMD_RECAL_4             0x14
#define ATA_CMD_RECAL_5             0x15
#define ATA_CMD_RECAL_6             0x16
#define ATA_CMD_RECAL_7             0x17
#define ATA_CMD_RECAL_8             0x18
#define ATA_CMD_RECAL_9             0x19
#define ATA_CMD_RECAL_A             0x1a
#define ATA_CMD_RECAL_B             0x1b
#define ATA_CMD_RECAL_C             0x1c
#define ATA_CMD_RECAL_D             0x1d
#define ATA_CMD_RECAL_E             0x1e
#define ATA_CMD_RECAL_F             0x1f

/*
    READ
*/

#define ATA_CMD_READ_SEC_RETRY      0x20    // read sector with retry
#define ATA_CMD_READ_SEC_NO_RETRY   0x21    // read sector without retry
#define ATA_CMD_READ_LONG_RETRY     0x22    // read long with retry
#define ATA_CMD_READ_LONG_NO_RETRY  0x23    // read long without retry
#define ATA_CMD_READ_SEC_EXT        0x24    // read sectors EXT
#define ATA_CMD_READ_DMA_EXT        0x25    // read DMA ext
#define ATA_CMD_READ_DMA_Q_EXT      0x26    // read DMA queued ext
#define ATA_CMD_READ_NTV_MAX        0x27    // read native max address
#define ATA_CMD_READ_MULTI_EXT      0x29    // read multiple EXT
#define ATA_CMD_READ_STRM_DMA       0x2a    // read stream DMA
#define ATA_CMD_READ_STREAM         0x2b    // read stream
#define ATA_CMD_READ_LOG_EXT        0x2f    // read log ext

#define ATA_CMD_READ_VRFY_RETRY     0x40    // read verify with retry
#define ATA_CMD_READ_VRFY_NO_RETRY  0x41    // read verify without retry
#define ATA_CMD_READ_VRFY_SEC_EXT   0x42    // read verify sector ext

#define ATA_CMD_READ_LOG_DMA_EXT    0x47    // read log DMA ext

/*
    WRITE
*/

#define ATA_CMD_WRITE_SEC_RETRY     0x30    // write sector with retry
#define ATA_CMD_WRITE_SEC_NO_RETRY  0x31    // write sector without retry
#define ATA_CMD_WRITE_LONG_RETRY    0x32    // write long with retry
#define ATA_CMD_WRITE_LONG_NO_RETRY 0x33    // write long without retry
#define ATA_CMD_WRITE_SEC_EXT       0x34    // write sectors EXT
#define ATA_CMD_WRITE_DMA_EXT       0x35    // write DMA ext
#define ATA_CMD_WRITE_DMA_Q_EXT     0x36    // write DMA queued ext
#define ATA_CMD_WRITE_NTV_MAX       0x37    // write native max address
#define ATA_CMD_CFA_RESERVED        0x38    // CFA reserved
#define ATA_CMD_WRITE_MULTI_EXT     0x39    // write multiple EXT
#define ATA_CMD_WRITE_STRM_DMA      0x3a    // write stream DMA
#define ATA_CMD_WRITE_STREAM        0x3b    // write stream
#define ATA_CMD_WRITE_VERIFY        0x3c    // write verify
#define ATA_CMD_WRITE_LOG_EXT       0x3f    // write log ext

#define ATA_CMD_WRITE_UNCORRECTABLE_EXT 0x45    // write uncorrectable ext

/*
    OTHER
*/

#define ATA_CMD_PACKET          0xa0    // packet
#define ATA_CMD_IDENT_PCK_DEV   0xa1    // identify packet device
#define ATA_CMD_IDENT_DEV       0xec    // identify device

typedef struct _ata_cmd
{
    uint16_t bus;               // drive bus (primary/secondary)
    union {
        uint8_t feature;        // 
        uint8_t error;          //
    };
    uint8_t count;              // number of sectors to read/write
    union {
        uint64_t lba;           // logical block address
        uint8_t lba_sep[4];     // logical block address split in 4
    };
    uint8_t device;             // device 
    union {
        uint8_t command;        // command to send
        uint8_t status;         // status to set 
    };
    uint8_t wait_status;        // 
} __attribute__((packed)) ata_cmd;