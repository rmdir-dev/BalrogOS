#pragma once

#include <stdint.h>

/*
    Offset from     Direction	Function	                        Description	                                                            Param. size LBA28/LBA48
    "I/O" base  
    0	            R/W	        Data Register	                    Read/Write PIO data bytes	                                            16-bit / 16-bit
    1	            R	        Error Register	                    Used to retrieve any error generated by the last ATA command executed.	8-bit / 16-bit
    1	            W	        Features Register	                Used to control command specific interface features.	                8-bit / 16-bit
    2	            R/W	        Sector Count Register	            Number of sectors to read/write (0 is a special value).	                8-bit / 16-bit
    3	            R/W	        Sector Number Register (LBAlo)	    This is CHS / LBA28 / LBA48 specific.	                                8-bit / 16-bit
    4	            R/W	        Cylinder Low Register / (LBAmid)	Partial Disk Sector address.	                                        8-bit / 16-bit
    5	            R/W	        Cylinder High Register / (LBAhi)	Partial Disk Sector address.	                                        8-bit / 16-bit
    6	            R/W	        Drive / Head Register	            Used to select a drive and/or head. Supports extra address/flag bits.	8-bit / 8-bit
    7	            R	        Status Register	                    Used to read the current status.	                                    8-bit / 8-bit
    7	            W	        Command Register	                Used to send ATA commands to the device.	                            8-bit / 8-bit
*/

#define ATA_DEV_IO_PREMARY      0x1f0   // Primary I/O base registers
#define ATA_DEV_IO_SECONDARY    0x170   // Secondary I/O base register

#define ATA_REG_W_DATA(reg)     (reg + 0)   // Write Data Register    Read/Write PIO data bytes
#define ATA_REG_W_FEATURE(reg)  (reg + 1)   // Write Features Register    Used to control command specific interface features.
#define ATA_REG_W_S_COUNT(reg)  (reg + 2)   // Write Sector Count Register    Number of sectors to read/write (0 is a special value).
#define ATA_REG_W_LBA_L(reg)    (reg + 3)   // Write Logical block addressing Low     This is CHS / LBA28 / LBA48 specific.
#define ATA_REG_W_LBA_M(reg)    (reg + 4)   // Write Logical block addressing mid     Partial Disk Sector address.
#define ATA_REG_W_LBA_H(reg)    (reg + 5)   // Write Logical block addressing high    Partial Disk Sector address.
#define ATA_REG_W_DEV(reg)      (reg + 6)   // Write Drive / Head Register    Used to select a drive and/or head. Supports extra address/flag bits.
#define ATA_REG_W_CMD(reg)      (reg + 7)   // Write Status Register	    Used to send ATA commands to the device.

#define ATA_REG_R_DATA(reg)     (reg + 0)   // Read Data Register    Read/Write PIO data bytes
#define ATA_REG_R_ERROR(reg)    (reg + 1)   // Read Error Register   Used to retrieve any error generated by the last ATA command executed.
#define ATA_REG_R_S_COUNT(reg)  (reg + 2)   // Read Sector Count Register    Number of sectors to read/write (0 is a special value).
#define ATA_REG_R_LBA_L(reg)    (reg + 3)   // Read Logical block addressing Low     This is CHS / LBA28 / LBA48 specific.
#define ATA_REG_R_LBA_M(reg)    (reg + 4)   // Read Logical block addressing mid     Partial Disk Sector address.
#define ATA_REG_R_LBA_H(reg)    (reg + 5)   // Read Logical block addressing high    Partial Disk Sector address.
#define ATA_REG_R_DEV(reg)      (reg + 6)   // Read Drive / Head Register    Used to select a drive and/or head. Supports extra address/flag bits.
#define ATA_REG_R_STATUS(reg)   (reg + 7)   // Read Status Register  Used to read the current status.

/*
    Offset from     Direction	Function	                Description	Param.                                                      size LBA28/LBA48
    "Control" base
    0	            R	        Alternate Status Register	A duplicate of the Status Register which does not affect interrupts.	8-bit / 8-bit
    0	            W	        Device Control Register	    Used to reset the bus or enable/disable interrupts.	                    8-bit / 8-bit
    1	            R	        Drive Address Register	    Provides drive select and head select information.	                    8-bit / 8-bit
*/

#define ATA_DEV_CTR_PRIMARY     0x3f6   // Primary control base register
#define ATA_DEV_CTR_SECONDARY   0x376   // Secondary control base register

/* MUST SUPPLY CTR REG
Read Alternate Status Register	A duplicate of the Status Register which does not affect interrupts. */
#define ATA_REG_R_CTR(reg)      (reg)

/* MUST SUPPLY CTR REG
 Write Device Control Register    Used to reset the bus or enable/disable interrupts.	 */
#define ATA_REG_W_CTR(reg)      (reg)

/*
    ERROR REG
    I/O base offset 1
*/

#define ATA_ERR_AMNF    0b00000001  // address mark not found
#define ATA_ERR_TKZNF   0b00000010  // track zero not found
#define ATA_ERR_ABRT    0b00000100  // aborted command
#define ATA_ERR_MCR     0b00001000  // media change request
#define ATA_ERR_IDNF    0b00010000  // ID not found
#define ATA_ERR_MC      0b00100000  // media changed
#define ATA_ERR_UNC     0b01000000  // uncorrectable data error
#define ATA_ERR_BBK     0b10000000  // bad block detected

/*
    DRIVE / HEAD REG (I/O base + 6)
*/
#define ATA_CHS_LBA_ADDR(reg)   (reg & 0b1111) // In CHS addressing, bits 0 to 3 of the head. In LBA addressing, bits 24 to 27 of the block number
#define ATA_DRV         0b00010000  // selected drive number
#define ATA_USE_LBA     0b01000000  // use LBA addressing if set, else CHS

/*
    STATUS REG (I/O base + 7)
*/

#define ATA_STATUS_ERR  0b00000001  // Indicates an error occurred. Send a new command to clear it (or nuke it with a Software Reset).
#define ATA_STATUS_IDX  0b00000010  // Index. Always set to zero.
#define ATA_STATUS_CORR 0b00000100  // Corrected data. Always set to zero.
#define ATA_STATUS_DRQ  0b00001000  // Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define ATA_STATUS_SRV  0b00010000  // Overlapped Mode Service Request.
#define ATA_STATUS_DF   0b00100000  // Drive Fault Error (does not set ERR).
#define ATA_STATUS_RDY  0b01000000  // Bit is clear when drive is spun down, or after an error. Set otherwise.
#define ATA_STATUS_BSY  0b10000000  // Indicates the drive is preparing to send/receive data (wait for it to clear). 
                                    // In case of 'hang' (it never clears), do a software reset.

/*
    CONTROL REGISTER (Control base + 0)
*/

#define ATA_CTR_nIEN    0b00000010  // Set this to stop the current device from sending interrupts.
#define ATA_CTR_SRST    0b00000100  // Set, then clear (after 5us), this to do a "Software Reset" on all ATA drives on a bus, if one is misbehaving.
#define ATA_CTR_HOB     0b10000000  // Set this to read back the High Order Byte of the last LBA48 value sent to an IO port.

/*
    DRIVE ADDRESS REGISTER (Control base + 1)
*/

#define ATA_DRV_ADDR_DS0    0b00000001  // Drive 0 select. Clears when drive 0 selected.
#define ATA_DRV_ADDR_DS1    0b00000010  // Drive 1 select. Clears when drive 1 selected.
#define ATA_DRV_ADDR_HS0    0b00000100  // One's compliment representation of the currently selected head.
#define ATA_DRV_ADDR_HS1    0b00001000  // One's compliment representation of the currently selected head.
#define ATA_DRV_ADDR_HS2    0b00010000  // One's compliment representation of the currently selected head.
#define ATA_DRV_ADDR_HS3    0b00100000  // One's compliment representation of the currently selected head.
#define ATA_DRV_ADDR_WTG    0b01000000  // Write gate; goes low while writing to the drive is in progress.

/*
    OTHER
*/

#define ATA_MASTER  0x00
#define ATA_SLAVE   0x10

#define ATAPI_LBA_MAGIC 0xEB1401

typedef struct _ata_id
{
    uint16_t config;                    //
    uint16_t UNUSED_FIELD_1[9];         // unused file
    char serial[20];                    //
    uint16_t UNUSED_FIELD_2[3];         // unused file
    char firmware[8];                   //
    char model[40];                     //
    uint8_t UNUSED_FIELD_3;             // unused file
    uint8_t sectors_per_interrupt;      //
    uint16_t UNUSED_FIELD_4;            // unused file
    uint16_t capabilities[2];           //
    uint16_t UNUSED_FIELD_5[2];         // unused file
    uint16_t validity;                  //
    uint16_t UNUSED_FIELD_6[3];         // unused file
    uint32_t capacity_sectors;          //
    uint16_t sectors_per_cmd;           //
    uint32_t capacity_lba28;            //
    uint16_t UNUSED_FIELD_7[38];        // unused file
    uint64_t capacity_lba48;            //
    uint16_t UNUSED_FIELD_8[152];       // unused file
} __attribute__((packed)) ata_id; 

typedef struct _ata_drive
{
    uint8_t exist;      // if the drive exist
    uint16_t io_bus;    // io bus port address
    uint16_t ctr_bus;   // control bus port address
    uint8_t master;     // 0x00 if master, 0x10 if slave
    uint8_t atapi;      // if the device is ATAPI (some changes with the ready bit)
    ata_id id;          // ata identification
}  __attribute__((packed)) ata_drive;