#pragma once

#include "balrog_os/file_system/filesystem.h"

#define PCI_USB_DEIVCE_TYPE_SERIAL_BUS      0x0c
#define PCI_USB_DEIVCE_TYPE_USB             0x03
#define PCI_USB_DEIVCE_TYPE_XHCI            0x30

#define XHCI_WAIT_WANT_BIT_UP               1
#define XHCI_WAIT_WANT_BIT_DOWN             0

#define XHCI_OP_READ_FAIL                   0xFFFFFFFF

/*
How much of the bar we map. we need 64KiB
*/
#define XHCI_BAR_PAGES                      16

/*
Where the bars are mapped, and it is not P2V.

P2V only reaches the first 256GiB of physical memory.
*/
#define XHCI_VIRTUAL_BASE                   0xFFFFFFFFB0000000

#define XHCI_PS_KEEP                        0xFF01FFFDU

#define XHCI_LEGSUP_CAPID                   0x01
#define XHCI_LEGSUP_BIOSOWNED               (1 << 16)
#define XHCI_LEGSUP_OSOWNED                 (1 << 24)

int init_xhci();
void xhci_select(void* controller);
void* xhci_current();
int xhci_scan_devices();
