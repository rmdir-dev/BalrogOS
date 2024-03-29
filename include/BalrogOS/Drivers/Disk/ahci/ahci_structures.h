#pragma once

#include "BalrogOS/Drivers/Bus/pci.h"
#include "ahci_command.h"

#define HBA_PxIS_CPDS		(1 << 31)	// Cold Port Detect Status
#define HBA_PxIS_TFES		(1 << 30)	// Task File Error Status
#define HBA_PxIS_HBFS		(1 << 29)	// Host Bus Fatal Error Status
#define HBA_PxIS_HBDS		(1 << 28)	// Host Bus Data Error Status
#define HBA_PxIS_IFS		(1 << 27)	// Interface Fatal Error Status 
#define HBA_PxIS_INFS		(1 << 26)	// Interface Non-fatal Error Status
#define HBA_PxIS_OFS		(1 << 24)	// Overflow Status
#define HBA_PxIS_IPMS		(1 << 23)	// Incorrect Port Multiplier Status 
#define HBA_PxIS_PRCS		(1 << 22)	// PhyRdy Change Status 
#define HBA_PxIS_DMPS		(1 << 7)	// Device Mechanical Presence Status
#define HBA_PxIS_PCS		(1 << 6)	// Port Connect Change Status 
#define HBA_PxIS_DPS		(1 << 5)	// Descriptor Processed
#define HBA_PxIS_UFS		(1 << 4)	// Unknown FIS Interrupt
#define HBA_PxIS_SDBS		(1 << 3)	// Set Device Bits Interrupt
#define HBA_PxIS_DDS		(1 << 2)	// DMA Setup FIS Interrupt
#define HBA_PxIS_PSS		(1 << 1)	// PIO Setup FIS Interrupt
#define HBA_PxIS_DHRS		(1)			// Device to Host Register FIS Interrupt 

typedef volatile struct _hba_port_t
{
	uint32_t clb;		    // 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		    // 0x04, command list base address upper 32 bits
	uint32_t fb;		    // 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		    // 0x0C, FIS base address upper 32 bits
	uint32_t is;		    // 0x10, interrupt status
	uint32_t ie;		    // 0x14, interrupt enable
	uint32_t cmd;		    // 0x18, command and status
	uint32_t rsv0;		    // 0x1C, Reserved
	uint32_t tfd;		    // 0x20, task file data
	uint32_t sig;		    // 0x24, signature
	uint32_t ssts;		    // 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		    // 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		    // 0x30, SATA error (SCR1:SError)
	uint32_t sact;		    // 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		    // 0x38, command issue
	uint32_t sntf;		    // 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		    // 0x40, FIS-based switch control
	uint32_t rsv1[11];	    // 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	    // 0x70 ~ 0x7F, vendor specific
} __attribute__((packed)) hba_port_t;

typedef volatile struct _hba_mem_t
{
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		    // 0x00, Host capability
	uint32_t ghc;		    // 0x04, Global host control
	uint32_t is;		    // 0x08, Interrupt status
	uint32_t pi;		    // 0x0C, Port implemented
                            // Each bit says if the port is set or not
                            // so 32 bit for 32 ports. 
	uint32_t vs;		    // 0x10, Version
	uint32_t ccc_ctl;	    // 0x14, Command completion coalescing control
	uint32_t ccc_pts;	    // 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t cap2;		    // 0x24, Host capabilities extended
	uint32_t bohc;		    // 0x28, BIOS/OS handoff control and status
 
	// 0x2C - 0x9F, Reserved
	uint8_t rsv[0xA0-0x2C];
 
	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t vendor[0x100-0xA0];
 
	// 0x100 - 0x10FF, Port control registers
	hba_port_t ports[1];	// 1 ~ 32
} __attribute__((packed)) hba_mem_t;

typedef struct __ahci_device_t 
{
    void* abar;                 // AHCI Base Memory Register
    pci_device_t* pci;          // Linked PCI device
    uint32_t key;               // hash map key

	uint8_t cap_64_bit;
	hba_port_t* port;
	uint32_t port_no;

	ahci_cmd_table_t* cmd_table;
	ahci_cmd_list_t* cmd_list;
	fis_device_reg_t* fis;

	uint8_t initialized;
} __attribute__((packed)) ahci_device_t;