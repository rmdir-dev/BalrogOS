#pragma once
/*
HEADER TYPE COMMON
*/

#define PCI_W_VENDOR_ID                 0x00
#define PCI_W_DEVICE_ID                 0x02
#define PCI_W_COMMAND                   0x04
#define PCI_W_STATUS                    0x06
#define PCI_B_REVISION_ID               0x08
#define PCI_B_PROG_IF                   0x09
#define PCI_B_SUBCLASS                  0x0a
#define PCI_B_CLASS_CODE                0x0b
#define PCI_B_CACHE_LINE_SIZE           0x0c
#define PCI_B_LATENCY_TIMER             0x0d
#define PCI_B_HEADER_TYPE               0x0e
#define PCI_B_BIST                      0x0f

/*
HEADER TYPE 0x00
*/
#define PCI_D_BASE_ADDRESS_0            0x10
#define PCI_D_BASE_ADDRESS_1            0x14
#define PCI_D_BASE_ADDRESS_2            0x18
#define PCI_D_BASE_ADDRESS_3            0x1c
#define PCI_D_BASE_ADDRESS_4            0x20
#define PCI_D_BASE_ADDRESS_5            0x24
#define PCI_D_CARDBUS_CIS_PTR           0x28
#define PCI_W_SUBSYSTEM_VENDOR_ID       0x2c
#define PCI_W_SUBSYSTEM_ID              0x2e
#define PCI_D_EXPANSION_ROM_BASE_ADDR   0x30
#define PCI_B_CAPABILITIES_PTR          0x34
#define PCI_B_INTERUPT_LINE             0x3c
#define PCI_B_INTERRUPT_PIN             0x3d
#define PCI_B_MIN_GRANT                 0x3e
#define PCI_B_MAX_GRANT                 0x3f

/*
HEADER TYPE 0x01
*/
#define PCI_B_PRIMARY_BUS_NUMBER        0x18
#define PCI_B_SECONDAY_BUS_NUMBER       0x19
#define PCI_B_SUBORDINATE_BUS_NUMBER    0x1a
#define PCI_B_SONCONDARY_LATENCY_TIMER  0x1b
#define PCI_B_IO_BASE                   0x1c
#define PCI_B_IO_LIMIT                  0x1d
#define PCI_W_SECONDARY_STATUS          0x1e
#define PCI_W_MEMORY_BASE               0x20
#define PCI_W_MEMORY_LIMIT              0x22
#define PCI_W_PREFETCHABLE_MEM_BASE     0x24
#define PCI_W_PREFETCHABLE_MEM_LIMIT    0x26
#define PCI_D_PREFETCHABLE_BASE_UPPER   0x28
#define PCI_D_PREFETCHABLE_LIMIT_UPPER  0x2c
#define PCI_W_IO_BASE_UPPER             0x30
#define PCI_W_IO_LIMIT_UPPER            0x32
#define PCI_D_EXPANSION_ROM_BASE_ADDR   0x38
#define PCI_W_BRIDGE_CONTROL            0x3e

/*
HEADER TYPE 0x02
*/