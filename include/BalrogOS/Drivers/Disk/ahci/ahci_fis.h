#pragma once

#include <stdint.h>

/*
Frame Information Structure
*/

#define FIS_TYPE_REG_H2D	    0x27 // Register FIS - host to device
#define FIS_TYPE_REG_D2H	    0x34 // Register FIS - device to host
#define FIS_TYPE_DMA_ACT	    0x39 // DMA activate FIS - device to host
#define FIS_TYPE_DMA_SETUP	    0x41 // DMA setup FIS - bidirectional
#define FIS_TYPE_DATA		    0x46 // Data FIS - bidirectional
#define FIS_TYPE_BIST		    0x58 // BIST activate FIS - bidirectional
#define FIS_TYPE_PIO_SETUP	    0x5F // PIO setup FIS - device to host
#define FIS_TYPE_DEV_BITS	    0xA1 // Set device bits FIS - device to host


typedef struct __fis_device_reg_t
{
	uint8_t fis_type;	            // FIS_TYPE
    uint8_t options;                // options

    union
    {
        uint8_t command;	        // Command register
        uint8_t status;             // status
    };

    union
    {
        uint8_t feature_low;        // Feature register
        uint8_t error;
    };

    uint8_t lba0;		            // LBA low register
    uint8_t lba1;		            // LBA mid register, 15:8
	uint8_t lba2;		            // LBA high register, 23:16
	uint8_t device;		            // Device register

	uint8_t lba3;		            // LBA register, 31:24
	uint8_t lba4;		            // LBA register, 39:32
	uint8_t lba5;		            // LBA register, 47:40
	uint8_t feature_high;	        // Feature register, 15:8

	uint8_t count_low;		        // Count register, 7:0
	uint8_t count_high;		        // Count register, 15:8
	uint8_t icc;		            // Isochronous command completion
	uint8_t control;	            // Control register

	uint8_t rsv1[4];	            // Reserved
} fis_device_reg_t;
