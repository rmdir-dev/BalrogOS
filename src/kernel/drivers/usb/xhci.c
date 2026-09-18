#include "balrog_os/drivers/usb/xhci/xhci.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/drivers/bus/pci_class.h"
#include "balrog_os/drivers/bus/pci.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include <string.h>

#include "balrog_os/cpu/tsc/tsc.h"
#include "balrog_os/drivers/bus/pci_command.h"
#include "balrog_os/drivers/disk/ahci/ahci_structures.h"
#include "balrog_os/drivers/usb/usb_spec.h"
#include "balrog_os/drivers/usb/xhci/xhci_registers.h"
#include "balrog_os/drivers/usb/xhci/xhci_structures.h"
#include "balrog_os/drivers/usb/usb_spec.h"
#include "balrog_os/drivers/usb/usb_storage.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/fs_devices.h"
#include "balrog_os/file_system/fs_config.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/file_system/filesystem.h"

#define XHCI_MAX_SLOTS      4
/*
Device context indexes run 1 to 31, dci = endpoint * 2 + direction, and the
control endpoint takes 0.
*/
#define XHCI_MAX_DCI        32

typedef struct __xhci_controller_t
{
    volatile uint8_t* cap;
    volatile uint8_t* op;
    volatile uint8_t* rt;
    volatile uint32_t* db;

    uint64_t* dcbaa;

    /*
    How many bytes the controller gives each context, from HCCPARAMS1.CSZ.
    */
    uint32_t ctx_size;

    xhci_ring_t cmd;
    xhci_ring_t event;
    xhci_erst_entry_t* erst;

    xhci_ring_t ep_ring[XHCI_MAX_SLOTS][XHCI_MAX_DCI];
} xhci_controller_t;

static xhci_controller_t* xhci = 0;

static list_t xhci_devices;
list_t usb_devices;
char usb_device_id = 'a';

int __usb_enumerate(uint8_t port, list_t* usb_devices);
int __scsi_read_capacity(usb_disk_t* disk);

extern int __scsi_rw10(usb_disk_t* disk, uint64_t lba, uint16_t blocks, void* buffer, uint8_t write);
extern int __usb_read(fs_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len);
extern int __usb_write(fs_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len);

void xhci_select(void* controller)
{
    xhci = (xhci_controller_t*) controller;
}

void* xhci_current()
{
    return xhci;
}

static uint8_t usb_bounce[USB_BOUNCE_SIZE] __attribute__((aligned(PAGE_SIZE)));


static xhci_slot_context_t* __xhci_in_slot(void* in)
{
    return (xhci_slot_context_t*)((uint8_t*) in + xhci->ctx_size);
}

static xhci_endpoint_context_t* __xhci_in_ep(void* in, uint8_t dci)
{
    return (xhci_endpoint_context_t*)((uint8_t*) in + xhci->ctx_size * (dci + 1));
}

static uint64_t __xhci_get_bar(pci_device_t* dev)
{
    // An xhci bar is 64 bit so we concat bar 1 (upper half) and bar 0 (lower half).
    return (((uint64_t) dev->bar[1]) << 32) | dev->bar[0];
}

static uint32_t __xhci_op_read(uint32_t reg)
{
    return *(volatile uint32_t*)(xhci->op + reg);
}

static void __xhci_op_write(uint32_t reg, uint32_t value)
{
    *(volatile uint32_t*)(xhci->op + reg) = value;
}

static void __xhci_op_write64(uint32_t reg, uint64_t value)
{
    *(volatile uint32_t*)(xhci->op + reg) = (uint32_t) value;
    *(volatile uint32_t*)(xhci->op + reg + 4) = (uint32_t)(value >> 32);
}

static uint32_t __xhci_rt_read(uint32_t reg)
{
    return *(volatile uint32_t*)(xhci->rt + reg);
}

static void __xhci_rt_write(uint32_t reg, uint32_t value)
{
    *(volatile uint32_t*)(xhci->rt + reg) = value;
}

static void __xhci_rt_write64(uint32_t reg, uint64_t value)
{
    *(volatile uint32_t*)(xhci->rt + reg) = (uint32_t) value;
    *(volatile uint32_t*)(xhci->rt + reg + 4) = (uint32_t)(value >> 32);
}

static uint32_t __xhci_cap_read(uint32_t reg)
{
    return *(volatile uint32_t*)((xhci->cap + XHCI_CAP_CAPLENGTH) + reg);
}

static int __xhci_ring_alloc(xhci_ring_t* ring)
{
    ring->physical = (uintptr_t) pmm_calloc();

    if(!ring->physical)
    {
        return -1;
    }

    ring->trbs = (xhci_trb_t*) P2V(ring->physical);
    ring->enqueue = 0;
    ring->dequeue = 0;

    /* the controller starts its own cycle at 1, so we have to agree */
    ring->cycle = 1;

    /* the last slot is the link back to the head, and TC tells the
       controller to flip its cycle when it follows it. XHCI_RING_USABLE is
       that last index. */
    ring->trbs[XHCI_RING_USABLE].parameter = ring->physical;
    ring->trbs[XHCI_RING_USABLE].status = 0;
    ring->trbs[XHCI_RING_USABLE].control = XHCI_TRB_TYPE(XHCI_TRB_LINK) | XHCI_TRB_TC | ring->cycle;

    return 0;
}

static int __xhci_map(pci_device_t* dev)
{
    uint64_t bar = __xhci_get_bar(dev);

    xhci = (xhci_controller_t*) vmalloc(sizeof(xhci_controller_t));

    if(!xhci)
    {
        return -1;
    }

    memset(xhci, 0, sizeof(xhci_controller_t));
    xhci->ctx_size = 32;

    /*
    bar = 0x00000002F7C0000C
    0x00000002F7C0000C & 0xFFFFFFFFFFFFFFF0
    = 0x00000002F7C00000

    the 0xC we drop is not address, it is what the hardware puts there :
    bit 0 = 0 memory bar, 1 io bar
    bits 2:1 = 00 32 bit, 10 64 bit
    bit 3 = prefetchable
    bits 63:4 = the address, it only starts here

    ULL and not U! ~0xFU as it is an unsigned int, 0xFFFFFFF0 widened
    with zeros so it would wipes bar1 and a controller mapped over 4GiB disappears.
    */
    bar &= ~0xFULL;

    /* a slot of its own in the xhci window. P2V is not usable here : a bar
       past P2V_MAX gives an address that belongs to something else, and the
       dell puts both of its controllers at 389GiB. */
    static size_t mapped = 0;

    xhci->cap = (volatile uint8_t*)(XHCI_VIRTUAL_BASE + (mapped * XHCI_BAR_PAGES * PAGE_SIZE));

    mapped++;

    for(size_t i = 0; i < XHCI_BAR_PAGES; i++)
    {
        vmm_set_page(0, (void*)((uintptr_t)xhci->cap + i * PAGE_SIZE),
                (void*)(uintptr_t)(bar + i * PAGE_SIZE),
                PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE);
    }
    /* CAPLENGTH is one byte at 0 */
    xhci->op = xhci->cap + (*xhci->cap);
    xhci->rt = xhci->cap + (__xhci_cap_read(XHCI_CAP_RTSOFF) & ~0x1FU);
    xhci->db = (volatile uint32_t*) (xhci->cap + (__xhci_cap_read(XHCI_CAP_DBOFF) & ~0x3U));

    /* the controller does dma, so it needs bus master. the firmware usually
       left it on, usually is not always. bit 2, see the warning below. */
    pci_set_command_register(dev, dev->command | (1 << 2) | PCI_CMD_MEMORY_SPACE);

    uint32_t hcs1 = __xhci_cap_read(XHCI_CAP_HCSPARAMS1);

    /* HCIVERSION is a 16 bit register at offset 2, so it comes back in the
       top half of a 32 bit read at 0. reading it at 2 would be unaligned. */
    kernel_debug_output(KDB_LVL_INFO, "xhci : version 0%x, %d ports, %d slots, caplength %d",
            __xhci_cap_read(XHCI_CAP_CAPLENGTH) >> 16,
            XHCI_HCS1_MAX_PORTS(hcs1), XHCI_HCS1_MAX_SLOTS(hcs1), *xhci->cap);

    return 0;
}

static int __xhci_wait_bit(uint32_t reg, uint32_t bit, uint8_t want, uint64_t ms)
{
    uint64_t deadline = tsc_read() + (ms * 10000) * tsc_per_100ns();

    while (1)
    {
        uint32_t value = __xhci_op_read(reg);

        if (value == XHCI_OP_READ_FAIL)
        {
            kernel_debug_output(KDB_LVL_CRITICAL, "xhci : controller is gone, reg 0%x", reg);
            return -1;
        }

        /*
        want is 0 or 1 : DOWN or UP
        value & bit gives 0 or the mask itself, not 0 or 1 : XHCI_STS_CNR
        is 1 << 11, so a set CNR reads back 0x800
        !! flattens whatever is left to 1, so it can be compared to want
        without it, want == (value & bit) would only ever work for bit 0

        waiting for CNR to come down, so bit = 0x800 and want = 0
        value = 0x801   & 0x800 = 0x800   !! = 1   1 == 0 -> keep waiting
        value = 0x001   & 0x800 = 0x000   !! = 0   0 == 0 -> done
        */
        if(want == !!(value & bit))
        {
            return 0;
        }

        if(tsc_read() > deadline)
        {
            kernel_debug_output(KDB_LVL_ERROR, "xhci : reg 0%x bit 0%x never went %d in %dms",
                    reg, bit, want, ms);
            return -1;
        }
    }
}

static int __xhci_reset()
{
    __xhci_op_write(XHCI_OP_USBCMD, __xhci_op_read(XHCI_OP_USBCMD) & ~XHCI_CMD_RUN);

    if(__xhci_wait_bit(XHCI_OP_USBSTS, XHCI_STS_HCH, XHCI_WAIT_WANT_BIT_UP, 20) != 0)
    {
        return -1;
    }

    __xhci_op_write(XHCI_OP_USBCMD, XHCI_CMD_HCRST);

    tsc_wait_100ns(10000);

    if(__xhci_wait_bit(XHCI_OP_USBCMD, XHCI_CMD_HCRST, XHCI_WAIT_WANT_BIT_DOWN, 500) != 0)
    {
        return -1;
    }

    if(__xhci_wait_bit(XHCI_OP_USBSTS, XHCI_STS_CNR, XHCI_WAIT_WANT_BIT_DOWN, 500) != 0)
    {
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "xhci : reset done, usbsts 0%x",
            __xhci_op_read(XHCI_OP_USBSTS));

    return 0;
}

static int __xhci_take_ownership()
{
    uint32_t offset = XHCI_HCC1_XECP(__xhci_cap_read(XHCI_CAP_HCCPARAMS1));

    while(offset)
    {
        volatile uint32_t* cap = (volatile uint32_t*)(xhci->cap + offset * 4);
        uint32_t value = *cap;

        if(XHCI_XECP_ID(value) == XHCI_XECP_LEGACY)
        {
            if(!(value & XHCI_LEGSUP_BIOSOWNED))
            {
                return 0;               /* nobody to take it from */
            }

            *cap = value | XHCI_LEGSUP_OSOWNED;

            /* and now wait for the firmware to let go. it answers an SMI to
               do it, so it is not instant. */
            uint64_t deadline = tsc_read() + (100 * 10000) * tsc_per_100ns();

            while(*cap & XHCI_LEGSUP_BIOSOWNED)
            {
                if(tsc_read() > deadline)
                {
                    kernel_debug_output(KDB_LVL_ERROR, "xhci : firmware kept the controller");
                    return -1;
                }
            }

            kernel_debug_output(KDB_LVL_INFO, "xhci : taken from the firmware");
            return 0;
        }

        /* next is a dword count relative to THIS capability, and 0 ends the
           list. adding it to the base instead of to offset walks in circles. */
        uint32_t next = XHCI_XECP_NEXT(value);

        if(!next)
        {
            break;
        }

        offset += next;
    }

    return 0;
}

static int __xhci_setup()
{
    uint32_t hcs1 = __xhci_cap_read(XHCI_CAP_HCSPARAMS1);
    uint32_t slots = XHCI_HCS1_MAX_SLOTS(hcs1);

    /* before any context is built */
    xhci->ctx_size = (__xhci_cap_read(XHCI_CAP_HCCPARAMS1) & XHCI_HCC1_CSZ) ? 64 : 32;

    kernel_debug_output(KDB_LVL_INFO, "xhci : %d slots, %d byte contexts", slots, xhci->ctx_size);

    /* tell it how many slots we will actually use. leaving CONFIG at 0 makes
       every Enable Slot fail with no slots available. */
    __xhci_op_write(XHCI_OP_CONFIG, slots);

    /* the dcbaa is indexed by slot number, so it needs slots + 1 entries :
       entry 0 is not a slot, it is where the scratchpad array goes. */
    xhci->dcbaa = (uint64_t*) P2V(pmm_calloc());

    if(!xhci->dcbaa)
    {
        return -1;
    }

    /* the scratchpad is memory the controller wants for itself. zero of them
       is common and legal, and a non zero count that we ignore gives a
       controller that resets itself the moment it runs. */
    uint32_t pads = XHCI_HCS2_MAX_SCRATCHPAD(__xhci_cap_read(XHCI_CAP_HCSPARAMS2));

    if(pads)
    {
        uint64_t* array = (uint64_t*) P2V(pmm_calloc());

        for(uint32_t i = 0; i < pads; i++)
        {
            array[i] = (uintptr_t) pmm_calloc();
        }

        xhci->dcbaa[0] = V2P(array);
        kernel_debug_output(KDB_LVL_INFO, "xhci : %d scratchpad pages", pads);
    }

    __xhci_op_write64(XHCI_OP_DCBAAP, V2P(xhci->dcbaa));

    /* the command ring, and RCS has to match the cycle we stamp */
    if(__xhci_ring_alloc(&xhci->cmd) != 0 || __xhci_ring_alloc(&xhci->event) != 0)
    {
        return -1;
    }

    __xhci_op_write64(XHCI_OP_CRCR, xhci->cmd.physical | XHCI_CRCR_RCS);

    /* the event ring is described to the controller by a table, not by a
       pointer : one segment is enough for us. */
    xhci->erst = (xhci_erst_entry_t*) P2V(pmm_calloc());
    xhci->erst[0].ring_base = xhci->event.physical;
    xhci->erst[0].ring_size = XHCI_RING_TRBS;

    /* size before base! writing ERSTBA is what makes the controller read the
       table, so a size of 0 still in place means it reads nothing. */
    __xhci_rt_write(XHCI_RT_IR_BASE + XHCI_IR_ERSTSZ, 1);
    __xhci_rt_write64(XHCI_RT_IR_BASE + XHCI_IR_ERDP, xhci->event.physical);
    __xhci_rt_write64(XHCI_RT_IR_BASE + XHCI_IR_ERSTBA, V2P(xhci->erst));

    return 0;
}

static int __xhci_start()
{
    __xhci_op_write(XHCI_OP_USBCMD, __xhci_op_read(XHCI_OP_USBCMD) | XHCI_CMD_RUN);

    /* HCH comes back down when it is really running */
    if(__xhci_wait_bit(XHCI_OP_USBSTS, XHCI_STS_HCH, XHCI_WAIT_WANT_BIT_DOWN, 100) != 0)
    {
        return -1;
    }

    uint32_t ports = XHCI_HCS1_MAX_PORTS(__xhci_cap_read(XHCI_CAP_HCSPARAMS1));
    uint32_t connected = 0;

    /* a port does not report a connection the instant the controller starts. */
    tsc_wait_100ns(1000000);

    for(uint32_t i = 0; i < ports; i++)
    {
        uint32_t sc = __xhci_op_read(XHCI_OP_PORT_BASE + i * XHCI_PORT_SIZE + XHCI_PORTSC);

        if(sc & XHCI_PORTSC_CCS)
        {
            connected++;
            kernel_debug_output(KDB_LVL_INFO, "xhci : port %d has something, portsc 0%x", i, sc);

            if(__usb_enumerate(i, &usb_devices) == 0)
            {
                break;
            }
        }
    }

    if(!connected)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : %d ports, none connected", ports);
    }

    kernel_debug_output(KDB_LVL_INFO, "xhci : running, usbsts 0%x", __xhci_op_read(XHCI_OP_USBSTS));
    return 0;
}

static int __xhci_probe_device(pci_device_t* dev)
{
    if (dev->subclass != PCI_SUBCLASS_SERIAL_USB_CONTROLLER || dev->prog_if != PCI_USB_DEIVCE_TYPE_XHCI)
    {
        return -1;
    }

    if (__xhci_map(dev) != 0)
    {
        return -1;
    }

    if(__xhci_take_ownership() != 0 || __xhci_reset() != 0
            || __xhci_setup() != 0 || __xhci_start() != 0)
    {
        vmfree(xhci);
        xhci = 0;
        return -1;
    }

    return 0;
}

static uint8_t __xhci_dma_ok(void* buffer)
{
    uintptr_t addr = (uintptr_t) buffer;

    return addr >= KERNEL_OFFSET && addr < FS_CACHE_OFFSET;
}

void __xhci_ring_push(xhci_ring_t* ring, xhci_trb_t* trb)
{
    xhci_trb_t* slot = &ring->trbs[ring->enqueue];

    slot->parameter = trb->parameter;
    slot->status = trb->status;

    slot->control = (trb->control & ~XHCI_TRB_CYCLE) | ring->cycle;

    ring->enqueue++;

    if(ring->enqueue == XHCI_RING_USABLE)
    {
        ring->trbs[ring->enqueue].control =
                (ring->trbs[ring->enqueue].control & ~XHCI_TRB_CYCLE) | ring->cycle;
        ring->cycle ^= 1;
        ring->enqueue = 0;
    }
}

void __xhci_doorbell(uint8_t slot, uint8_t dci)
{
    /* one dword per slot, and slot 0 is the command ring. */
    xhci->db[slot] = dci;
}

int __xhci_wait_event(xhci_trb_t* out, uint32_t type, uint64_t ms)
{
    uint64_t deadline = tsc_read() + (ms * 10000) * tsc_per_100ns();

    while(1)
    {
        xhci_trb_t* trb = &xhci->event.trbs[xhci->event.dequeue];

        if(!!(trb->control & XHCI_TRB_CYCLE) == xhci->event.cycle)
        {
            *out = *trb;

            xhci->event.dequeue++;

            /* XHCI_RING_TRBS and not USABLE : an event ring has no link trb,
               the controller wraps it by size and so do we. */
            if(xhci->event.dequeue == XHCI_RING_TRBS)
            {
                xhci->event.dequeue = 0;
                xhci->event.cycle ^= 1;
            }

            /* tell the controller where we stopped, and clear EHB in the same
               write. leaving EHB set means it never raises another event. */
            __xhci_rt_write64(XHCI_RT_IR_BASE + XHCI_IR_ERDP,
                    (xhci->event.physical + xhci->event.dequeue * XHCI_TRB_SIZE)
                    | XHCI_ERDP_EHB);

            if(XHCI_TRB_GET_TYPE(out->control) != type)
            {
                continue;
            }

            /*  bits 31:24 of status, 1 is success  */
            return (out->status >> 24) & 0xFF;
        }

        if(tsc_read() > deadline)
        {
            kernel_debug_output(KDB_LVL_ERROR, "xhci : no type %d event after %dms", type, ms);
            return -1;
        }
    }
}

xhci_ring_t* __xhci_ring_for(uint8_t slot, uint8_t dci)
{
    if(slot >= XHCI_MAX_SLOTS || dci >= XHCI_MAX_DCI)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : no ring for slot %d dci %d", slot, dci);
        return 0;
    }

    return &xhci->ep_ring[slot][dci];
}

int __xhci_command(xhci_trb_t* trb, xhci_trb_t* event)
{
    __xhci_ring_push(&xhci->cmd, trb);
    __xhci_doorbell(0, 0);

    int code = __xhci_wait_event(event, XHCI_TRB_COMMAND_EVENT, 500);

    if(code != XHCI_COMP_SUCCESS)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : command type %d refused, code %d",
                XHCI_TRB_GET_TYPE(trb->control), code);
    }

    return code;
}

int __xhci_enable_slot(uint8_t* out)
{
    xhci_trb_t trb = {};
    xhci_trb_t event = {};

    trb.control = XHCI_TRB_TYPE(XHCI_TRB_ENABLE_SLOT);

    if(__xhci_command(&trb, &event) != XHCI_COMP_SUCCESS)
    {
        return -1;
    }

    /*  the controller picks the slot, we do not. it is in the top byte of the
        event control word.  */
    *out = (event.control >> 24) & 0xFF;

    kernel_debug_output(KDB_LVL_INFO, "xhci : slot %d enabled", *out);
    return 0;
}

int __xhci_address_device(uint8_t slot, uint32_t port)
{
    xhci_trb_t trb = {};
    xhci_trb_t event = {};
    xhci_ring_t* ring = __xhci_ring_for(slot, XHCI_DCI_CONTROL);

    if(!ring || __xhci_ring_alloc(ring) != 0)
    {
        return -1;
    }

    xhci_input_context_t* in = (xhci_input_context_t*) P2V(pmm_calloc());

    if(!in)
    {
        return -1;
    }

    if(!xhci->dcbaa[slot])
    {
        void* out = pmm_calloc();

        if(!out)
        {
            return -1;
        }

        xhci->dcbaa[slot] = (uintptr_t) out;
    }

    in->control.add_flags = 0x03;

    uint32_t sc = __xhci_op_read(XHCI_OP_PORT_BASE + port * XHCI_PORT_SIZE + XHCI_PORTSC);

    __xhci_in_slot(in)->dword0 = XHCI_SLOT_CTX_ENTRIES(1) | XHCI_SLOT_SPEED(XHCI_PORTSC_SPEED(sc));

    __xhci_in_slot(in)->dword1 = XHCI_SLOT_ROOT_PORT(port + 1);

    __xhci_in_ep(in, XHCI_DCI_CONTROL)->dword1 = XHCI_EP_TYPE(XHCI_EP_CONTROL) | XHCI_EP_ERROR_COUNT(3) | XHCI_EP_MAX_PACKET(8);

    __xhci_in_ep(in, XHCI_DCI_CONTROL)->dequeue = ring->physical | ring->cycle;

    trb.parameter = V2P(in);
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_ADDRESS_DEVICE) | ((uint32_t) slot << 24);

    return (__xhci_command(&trb, &event) == XHCI_COMP_SUCCESS) ? 0 : -1;
}

int __xhci_control_transfer(uint8_t slot, usb_setup_packet_t* setup, void* buffer, uint16_t len)
{
    xhci_ring_t* ring = __xhci_ring_for(slot, XHCI_DCI_CONTROL);
    xhci_trb_t trb = {};
    uint32_t trt = XHCI_TRT_NO_DATA;

    if(!ring)
    {
        return -1;
    }

    if(len)
    {
        trt = (setup->request_type & USB_DIR_IN) ? XHCI_TRT_IN_DATA : XHCI_TRT_OUT_DATA;
    }

    /*  SETUP STAGE  */
    memcpy(&trb.parameter, setup, sizeof(usb_setup_packet_t));
    trb.status = 8;             /* the setup packet is always 8 bytes long */
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_SETUP_STAGE) | XHCI_TRB_IDT | trt;
    __xhci_ring_push(ring, &trb);

    /*  DATA STAGE, only when there is something to carry  */
    if(len)
    {
        trb.parameter = V2P(buffer);
        trb.status = len;
        trb.control = XHCI_TRB_TYPE(XHCI_TRB_DATA_STAGE);

        if(setup->request_type & USB_DIR_IN)
        {
            trb.control |= XHCI_TRB_DIR_IN;
        }

        __xhci_ring_push(ring, &trb);
    }

    /*  STATUS STAGE. */
    trb.parameter = 0;
    trb.status = 0;
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_STATUS_STAGE) | XHCI_TRB_IOC;

    if(!len || !(setup->request_type & USB_DIR_IN))
    {
        /* bit 16, to add to the header */
        trb.control |= XHCI_TRB_DIR_IN;
    }

    __xhci_ring_push(ring, &trb);

    __xhci_doorbell(slot, XHCI_DCI_CONTROL);

    xhci_trb_t event = {};
    int code = __xhci_wait_event(&event, XHCI_TRB_TRANSFER_EVENT, 500);

    /*  same reason as __xhci_command : the callers collapse everything into a
        plain -1, so the completion code has to be said here or it is lost.  */
    if(code != XHCI_COMP_SUCCESS && code != XHCI_COMP_SHORT_PACKET)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : slot %d control transfer refused, code %d, len %d", slot, code, len);
    }

    return code;
}

int __xhci_port_reset(uint32_t port)
{
    uint32_t reg = XHCI_OP_PORT_BASE + port * XHCI_PORT_SIZE + XHCI_PORTSC;
    uint32_t sc = __xhci_op_read(reg);

    __xhci_op_write(reg, (sc & XHCI_PS_KEEP) | XHCI_PORTSC_PR);

    if(__xhci_wait_bit(reg, XHCI_PORTSC_PR, XHCI_WAIT_WANT_BIT_DOWN, 200) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : port %d never finished its reset, portsc 0%x",
                port, __xhci_op_read(reg));
        return -1;
    }

    if(!(__xhci_op_read(reg) & XHCI_PORTSC_PED))
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : port %d reset but not enabled, portsc 0%x",
                port, __xhci_op_read(reg));
        return -1;
    }

    return 0;
}

int __xhci_evaluate_context(uint8_t slot, uint16_t max_packet)
{
    xhci_trb_t trb = {};
    xhci_trb_t event = {};
    xhci_input_context_t* in = (xhci_input_context_t*) P2V(pmm_calloc());

    if(!in)
    {
        return -1;
    }

    in->control.add_flags = 0x02;

    __xhci_in_ep(in, XHCI_DCI_CONTROL)->dword1 = XHCI_EP_TYPE(XHCI_EP_CONTROL)
            | XHCI_EP_ERROR_COUNT(3) | XHCI_EP_MAX_PACKET(max_packet);

    trb.parameter = V2P(in);
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_EVAL_CONTEXT) | ((uint32_t) slot << 24);

    return (__xhci_command(&trb, &event) == XHCI_COMP_SUCCESS) ? 0 : -1;
}

int __xhci_configure_endpoints(usb_disk_t* disk)
{
    xhci_trb_t trb = {};
    xhci_trb_t event = {};
    xhci_input_context_t* in = (xhci_input_context_t*) P2V(pmm_calloc());

    if(!in)
    {
        return -1;
    }

    uint8_t highest = (disk->bulk_in_dci > disk->bulk_out_dci)
            ? disk->bulk_in_dci : disk->bulk_out_dci;

    uint32_t sc = __xhci_op_read(XHCI_OP_PORT_BASE + disk->port * XHCI_PORT_SIZE + XHCI_PORTSC);

    in->control.add_flags = (1 << disk->bulk_in_dci) | (1 << disk->bulk_out_dci) | 1;
    __xhci_in_slot(in)->dword0 = XHCI_SLOT_CTX_ENTRIES(highest) | XHCI_SLOT_SPEED(XHCI_PORTSC_SPEED(sc));
    __xhci_in_slot(in)->dword1 = XHCI_SLOT_ROOT_PORT(disk->port + 1);

    uint8_t dcis[2] = { disk->bulk_out_dci, disk->bulk_in_dci };
    uint8_t types[2] = { XHCI_EP_BULK_OUT, XHCI_EP_BULK_IN };

    for(uint8_t i = 0; i < 2; i++)
    {
        xhci_ring_t* ring = __xhci_ring_for(disk->slot, dcis[i]);

        if(!ring || __xhci_ring_alloc(ring) != 0)
        {
            return -1;
        }

        xhci_endpoint_context_t* ep = __xhci_in_ep(in, dcis[i]);

        ep->dword1 = XHCI_EP_TYPE(types[i]) | XHCI_EP_ERROR_COUNT(3)
                | XHCI_EP_MAX_PACKET(512);
        ep->dequeue = ring->physical | ring->cycle;
    }

    trb.parameter = V2P(in);
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_CONFIG_EP) | ((uint32_t) disk->slot << 24);

    return (__xhci_command(&trb, &event) == XHCI_COMP_SUCCESS) ? 0 : -1;
}

int init_xhci()
{
    KERNEL_LOG_INFO("Looking for XHCI devices");

    list_init(&xhci_devices);
    list_init(&usb_devices);

    // Get all pci MASS STORAGES devices
    list_t* list = pci_get_devices(PCI_CLASS_SERIAL_BUS_CONTROLLER);
    list_node_t* node = list->head;
    int devices_found = 0;
    while(node)
    {
        pci_device_t* dev = node->value;
        node = node->next;
        if(__xhci_probe_device(dev) != 0)
        {
            continue;
        }

        list_insert(&xhci_devices, dev->key, dev);
        devices_found++;
    }

    if (devices_found == 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "xhci : no controller on the bus");
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "xhci : controller ready");

    return 0;
}

int __xhci_bulk(usb_disk_t* disk, uint8_t dci, void* buffer,
        uint32_t len)
{
    xhci_trb_t trb = {};
    xhci_trb_t event = {};
    uint8_t direct = __xhci_dma_ok(buffer);
    void* dma = buffer;

    if(!direct)
    {
        if(len > USB_BOUNCE_SIZE)
        {
            kernel_debug_output(KDB_LVL_ERROR, "usb : %d bytes will not fit the bounce buffer", len);
            return -1;
        }

        dma = usb_bounce;

        if(dci == disk->bulk_out_dci)
        {
            memcpy(usb_bounce, buffer, len);
        }
    }

    trb.parameter = V2P(dma);
    trb.status = len;
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_NORMAL) | XHCI_TRB_IOC | XHCI_TRB_ISP;

    xhci_ring_t* ring = __xhci_ring_for(disk->slot, dci);

    if(!ring)
    {
        return -1;
    }

    __xhci_ring_push(ring, &trb);
    __xhci_doorbell(disk->slot, dci);

    int code = __xhci_wait_event(&event, XHCI_TRB_TRANSFER_EVENT, 5000);

    if(code != XHCI_COMP_SUCCESS && code != XHCI_COMP_SHORT_PACKET)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : bulk on dci %d failed, code %d", dci, code);
        return -1;
    }

    if(!direct && dci == disk->bulk_in_dci)
    {
        memcpy(buffer, usb_bounce, len);
    }

    return 0;
}

int __xhci_bulk_out(usb_disk_t* disk, void* buffer, uint32_t len)
{
    return __xhci_bulk(disk, disk->bulk_out_dci, buffer, len);
}

int __xhci_bulk_in(usb_disk_t* disk, void* buffer, uint32_t len)
{
    return __xhci_bulk(disk, disk->bulk_in_dci, buffer, len);
}

int xhci_scan_devices()
{
    uint16_t* buffer = vmalloc(ATA_SECTOR_SIZE);
    int found = 0;

    if(!buffer)
    {
        return -1;
    }

    for(list_node_t* node = usb_devices.head; node; node = node->next)
    {
        usb_disk_t* xhci_disk = (usb_disk_t*) node->value;
        if(!xhci_disk->block_size)
        {
            kernel_debug_output(KDB_LVL_ERROR, "xhci : no disk enumerated, no usb device");
            continue;
        }

        KERNEL_LOG_INFO("searching boot device. slot %d port %d", xhci_disk->slot, xhci_disk->port);

        if(__scsi_rw10(xhci_disk, 0, 1, buffer, USB_READ) == 0)
        {
            fs_device_t* device = vmalloc(sizeof(fs_device_t));
            device->name = vmalloc(5 + 1); // usba 4 char + nullbyte + buffer
            fs_device_init(device);
            memcpy(device->name, "usb", 3);
            device->name[3] = usb_device_id++;
            device->name[4] = 0;
            device->type = FS_DEVICE_TYPE_XHCI;
            device->unique_id = xhci_disk->slot;
            device->read = __usb_read;
            device->write = __usb_write;
            device->drive = xhci_disk;
            device->first_lba = 0;
            fs_add_device(device);
            found++;
        }
    }

    vmfree(buffer);

    return found ? 0 : -1;
}
