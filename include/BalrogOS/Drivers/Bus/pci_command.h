#pragma once

#define PCI_CMD_IO_SPACE                    0x01 << 0
#define PCI_CMD_MEMORY_SPACE                0x01 << 1
#define PCI_CMD_SPECIAL_CYCLE               0x01 << 2
#define PCI_CMD_MEM_WRITE_AND_INVALIDATE    0x01 << 3
#define PCI_CMD_VGA_PALETTE_SNOOP           0x01 << 4
#define PCI_CMD_PARITY_ERR_RESP             0x01 << 5
#define PCI_CMD_SERR_ENABLE                 0x01 << 8
#define PCI_CMD_FAST_BACK_TO_BACK           0x01 << 9
#define PCI_CMD_INTERRUPT_DISABLE           0x01 << 10

#define PCI_STS_INTERRUPT                   0x01 << 3
#define PCI_STS_CAPABILITIES_LIST           0x01 << 4
#define PCI_STS_66_MHz_CAPABILITY           0x01 << 5
#define PCI_STS_FAST_BACK_TO_BACK_CAPABLE   0x01 << 7
#define PCI_STS_MASTER_DATA_PARITY_ERR      0x01 << 8
#define PCI_STS_DEVSEL_TIMING               0x01 << 9 // 2 bits
#define PCI_STS_SIGNAL_TARGET_ABORT         0x01 << 11
#define PCI_STS_RECEIVE_TARGET_ABORT        0x01 << 12
#define PCI_STS_RECEIVE_MASTER_ABORT        0x01 << 13
#define PCI_STS_SIGNALED_SYSTEM_ERROR       0x01 << 14
#define PCI_STS_DETECTED_PARITY_ERROR       0x01 << 15