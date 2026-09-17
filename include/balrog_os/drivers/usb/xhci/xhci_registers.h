#pragma once

#include <stdint.h>

/*
xHCI registers.

References :
xHCI      : https://wiki.osdev.org/EXtensible_Host_Controller_Interface
xHCI spec : chapter 5, Host Controller Register Interface
*/

/*
Capability registers, at the base. Read once at init, three of them are how we
find the other spaces.
*/
#define XHCI_CAP_CAPLENGTH      0x00    // 1 byte, where the operational registers start
#define XHCI_CAP_HCIVERSION     0x02    // 2 bytes, interface version, 0x0100 for 1.0
#define XHCI_CAP_HCSPARAMS1     0x04    // max slots, max interrupters, max ports
#define XHCI_CAP_HCSPARAMS2     0x08    // erst max, and the scratchpad buffer count
#define XHCI_CAP_HCSPARAMS3     0x0C    // exit latencies, of no use to us
#define XHCI_CAP_HCCPARAMS1     0x10    // capabilities, and where the extended list is
#define XHCI_CAP_DBOFF          0x14    // offset of the doorbell registers
#define XHCI_CAP_RTSOFF         0x18    // offset of the runtime registers

#define XHCI_HCS1_MAX_SLOTS(p)  ((p) & 0xFF)            // how many device slots
#define XHCI_HCS1_MAX_PORTS(p)  (((p) >> 24) & 0xFF)    // how many root ports

/*
The scratchpad count comes in two halves that aren't next to each other.
*/
#define XHCI_HCS2_MAX_SCRATCHPAD(p) (((((p) >> 21) & 0x1F) << 5) | (((p) >> 27) & 0x1F))

#define XHCI_HCC1_AC64          (1 << 0)    // 64 bit addressing, see the warning below
#define XHCI_HCC1_CSZ           (1 << 2)    // contexts are 64 bytes instead of 32
#define XHCI_HCC1_XECP(p)       (((p) >> 16) & 0xFFFF)  // extended capabilities, in dwords

/*
Operational registers, at base + CAPLENGTH.
*/
#define XHCI_OP_USBCMD          0x00    // run/stop, reset, interrupt enable
#define XHCI_OP_USBSTS          0x04    // halted, not ready, errors
#define XHCI_OP_PAGESIZE        0x08    // page size the controller wants, as a bitmap
#define XHCI_OP_DNCTRL          0x14    // device notification control
#define XHCI_OP_CRCR            0x18    // command ring control, reads back as 0
#define XHCI_OP_DCBAAP          0x30    // device context base address array pointer
#define XHCI_OP_CONFIG          0x38    // how many slots we enable

#define XHCI_CMD_RUN            (1 << 0)    // Run/Stop
#define XHCI_CMD_HCRST          (1 << 1)    // Host Controller Reset
#define XHCI_CMD_INTE           (1 << 2)    // Interrupter Enable
#define XHCI_CMD_HSEE           (1 << 3)    // Host System Error Enable

#define XHCI_STS_HCH            (1 << 0)    // Host Controller Halted
#define XHCI_STS_HSE            (1 << 2)    // Host System Error
#define XHCI_STS_EINT           (1 << 3)    // Event Interrupt
#define XHCI_STS_PCD            (1 << 4)    // Port Change Detect
#define XHCI_STS_CNR            (1 << 11)   // Controller Not Ready
#define XHCI_STS_HCE            (1 << 12)   // Host Controller Error

#define XHCI_CRCR_RCS           (1 << 0)    // Ring Cycle State
#define XHCI_CRCR_CS            (1 << 1)    // Command Stop
#define XHCI_CRCR_CA            (1 << 2)    // Command Abort
#define XHCI_CRCR_CRR           (1 << 3)    // Command Ring Running

/*
Port registers, four dwords each, at operational + 0x400.
*/
#define XHCI_OP_PORT_BASE       0x400
#define XHCI_PORT_SIZE          0x10

#define XHCI_PORTSC             0x00    // status and control
#define XHCI_PORTPMSC           0x04    // power management
#define XHCI_PORTLI             0x08    // link info
#define XHCI_PORTHLPMC          0x0C    // hardware LPM control

#define XHCI_PORTSC_CCS         (1 << 0)    // Current Connect Status
#define XHCI_PORTSC_PED         (1 << 1)    // Port Enabled/Disabled
#define XHCI_PORTSC_OCA         (1 << 3)    // Over-current Active
#define XHCI_PORTSC_PR          (1 << 4)    // Port Reset, hardware clears it when done
#define XHCI_PORTSC_PP          (1 << 9)    // Port Power
#define XHCI_PORTSC_CSC         (1 << 17)   // Connect Status Change
#define XHCI_PORTSC_PEC         (1 << 18)   // Port Enabled/Disabled Change
#define XHCI_PORTSC_PRC         (1 << 21)   // Port Reset Change

#define XHCI_PORTSC_PLS(p)      (((p) >> 5) & 0x0F)     // Port Link State
#define XHCI_PORTSC_SPEED(p)    (((p) >> 10) & 0x0F)    // the speed the device came up at

/*
The change bits 17 to 23 are write 1 to clear! A plain portsc |= PR reads the
register, finds them set, and clears them on the way out. Mask them first :

    portsc = (portsc & XHCI_PORTSC_RW1C_MASK) | XHCI_PORTSC_PR;

Same trap as hba->is and port->is in the ahci.
*/
#define XHCI_PORTSC_RW1C_MASK   0x80FF00F7

#define XHCI_SPEED_FULL         1   // 12 Mbit/s, usb 1.1
#define XHCI_SPEED_LOW          2   // 1.5 Mbit/s, usb 1.0
#define XHCI_SPEED_HIGH         3   // 480 Mbit/s, usb 2.0
#define XHCI_SPEED_SUPER        4   // 5 Gbit/s, usb 3.0

#define XHCI_RT_MFINDEX         0x0000  // microframe counter, unused here
#define XHCI_RT_IR_BASE         0x0020  // first interrupter
#define XHCI_RT_IR_SIZE         0x0020  // one interrupter takes 32 bytes

#define XHCI_IR_IMAN            0x00    // interrupter management, pending and enable
#define XHCI_IR_IMOD            0x04    // interrupt moderation
#define XHCI_IR_ERSTSZ          0x08    // how many entries the erst holds
#define XHCI_IR_ERSTBA          0x10    // erst base address, 64 bits
#define XHCI_IR_ERDP            0x18    // event ring dequeue pointer, 64 bits

#define XHCI_IMAN_IP            (1 << 0)    // Interrupt Pending, write one to clear
#define XHCI_IMAN_IE            (1 << 1)    // Interrupt Enable

/*
The low bits of ERDP aren't part of the address. EHB acknowledges the events we
just read and it has to be written back, or the ring stops filling : first
event arrives, second one never does.
*/
#define XHCI_ERDP_DESI_MASK     0x07        // dequeue erst segment index
#define XHCI_ERDP_EHB           (1 << 3)    // Event Handler Busy

/*
Doorbell registers, at base + DBOFF. One dword per slot, and slot 0 is the
command ring, not a device.
*/
#define XHCI_DB_COMMAND_RING    0       // ring this one with 0 to run a command
#define XHCI_DB_TARGET(dci)     (dci)   // ring doorbell[slot] with the endpoint dci

/*
Extended capabilities, a linked list starting from HCCPARAMS1. Only one of them
matters here, it's how we take the controller from the firmware.
*/
#define XHCI_XECP_ID(x)         ((x) & 0xFF)
#define XHCI_XECP_NEXT(x)       (((x) >> 8) & 0xFF)     // in dwords, 0 ends the list

#define XHCI_XECP_LEGACY        0x01    // USB Legacy Support capability
#define XHCI_XECP_PROTOCOL      0x02    // Supported Protocol, maps usb2 and usb3 ports

#define XHCI_LEGSUP_BIOSOWNED   (1 << 16)   // set by the firmware, we wait for it to clear
#define XHCI_LEGSUP_OSOWNED     (1 << 24)   // we set this one to claim the controller

/*
Port map of the Supported Protocol capability. A controller saying 8 ports
usually means 4 usb3 and 4 usb2, and the same physical socket shows up twice
under two numbers depending on the speed the device came up at.
*/
#define XHCI_PROTOCOL_PORT_OFFSET(x)    ((x) & 0xFF)        // first port of this protocol
#define XHCI_PROTOCOL_PORT_COUNT(x)     (((x) >> 8) & 0xFF) // how many follow it

/*
The only space whose layout is fixed. The rest is base + offset, computed once.
*/
typedef volatile struct __xhci_cap_regs_t
{
    uint8_t caplength;      // 0x00, where the operational registers begin
    uint8_t reserved;       // 0x01
    uint16_t hciversion;    // 0x02, bcd, 0x0100 is xhci 1.0
    uint32_t hcsparams1;    // 0x04, max slots, interrupters and ports
    uint32_t hcsparams2;    // 0x08, erst max and scratchpad count
    uint32_t hcsparams3;    // 0x0C, exit latencies
    uint32_t hccparams1;    // 0x10, capabilities and the extended list offset
    uint32_t dboff;         // 0x14, doorbell offset
    uint32_t rtsoff;        // 0x18, runtime offset
    uint32_t hccparams2;    // 0x1C, more capabilities, none we need
} __attribute__((packed)) xhci_cap_regs_t;

/*
One port, repeated MaxPorts times.
*/
typedef volatile struct __xhci_port_regs_t
{
    uint32_t portsc;        // 0x00, connect status, reset, speed, change bits
    uint32_t portpmsc;      // 0x04, power management
    uint32_t portli;        // 0x08, link info
    uint32_t porthlpmc;     // 0x0C, hardware controlled low power
} __attribute__((packed)) xhci_port_regs_t;

/*
One interrupter. We only ever use the first.
*/
typedef volatile struct __xhci_interrupter_regs_t
{
    uint32_t iman;          // 0x00, interrupt pending and enable
    uint32_t imod;          // 0x04, moderation
    uint32_t erstsz;        // 0x08, number of erst entries, 1 for us
    uint32_t reserved;      // 0x0C
    uint64_t erstba;        // 0x10, physical address of the erst
    uint64_t erdp;          // 0x18, dequeue pointer, plus DESI and EHB in the low bits
} __attribute__((packed)) xhci_interrupter_regs_t;
