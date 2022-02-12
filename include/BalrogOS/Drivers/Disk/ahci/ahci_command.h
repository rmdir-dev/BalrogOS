#pragma once

#include <stdint.h>
#include "ahci_fis.h"
#include "BalrogOS/Drivers/Disk/ata/ata_command.h"

typedef struct __ahci_cmd_header_t
{
	uint8_t cflen:5;		// Command FIS length in DWORDS, 2 ~ 16
	uint8_t a:1;		    // ATAPI
	uint8_t w:1;		    // Write, 1: H2D, 0: D2H
	uint8_t p:1;		    // Prefetchable
 
	uint8_t r:1;		    // Reset
	uint8_t b:1;		    // BIST
	uint8_t c:1;		    // Clear busy upon R_OK
	uint8_t rsv0:1;		    // Reserved
	uint8_t pmp:4;		    // Port multiplier port
 
	uint16_t prdtlen;		// Physical region descriptor table length in entries
 
	uint32_t prdbc;		    // Physical region descriptor byte count transferred

	uint32_t ctba;		    // Command table descriptor base address
	uint32_t ctbau;		    // Command table descriptor base address upper 32 bits

	uint32_t rsv1[4];	    // Reserved
} __attribute__((packed)) ahci_cmd_header_t;

typedef struct __ahci_cmd_list_t
{
    ahci_cmd_header_t cmd[32];
} __attribute__((packed)) ahci_cmd_list_t;

typedef struct __ahci_prdt_entry_t
{
	uint32_t dba;		// Data base address
	uint32_t dbau;		// Data base address upper 32 bits
	uint32_t rsv0;		// Reserved
 
	uint32_t dbc:22;    // Byte count, 4M max
	uint32_t rsv1:9;    // Reserved
	uint32_t i:1;		// Interrupt on completion
} __attribute__((packed)) ahci_prdt_entry_t;

typedef struct __ahci_cmd_table_t
{
	uint8_t cfis[64];	// Command FIS
 
	uint8_t acmd[16];	// ATAPI command, 12 or 16 bytes
	
	uint8_t rsv[48];	// Reserved

	ahci_prdt_entry_t prdt_entry[32];   // Physical region descriptor table entries, 0 ~ 65535
} __attribute__((packed)) ahci_cmd_table_t;