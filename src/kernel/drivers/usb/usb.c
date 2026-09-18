#include "balrog_os/drivers/usb/usb_spec.h"
#include "balrog_os/drivers/usb/usb_storage.h"
#include "balrog_os/drivers/usb/xhci/xhci.h"
#include "balrog_os/drivers/usb/xhci/xhci_structures.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/cpu/tsc/tsc.h"
#include "klib/data_structure/list.h"
#include <string.h>

#include "balrog_os/memory/kheap.h"

extern int __xhci_control_transfer(uint8_t slot, usb_setup_packet_t* setup, void* buffer, uint16_t len);
extern int __xhci_configure_endpoints(usb_disk_t* disk);
extern int __xhci_enable_slot(uint8_t* out);
extern int __xhci_address_device(uint8_t slot, uint32_t port);
extern int __xhci_evaluate_context(uint8_t slot, uint16_t max_packet);
extern int __xhci_port_reset(uint32_t port);
extern void* xhci_current();
extern void xhci_select(void* controller);
extern int __scsi_read_capacity(usb_disk_t* disk);
extern int __xhci_command(xhci_trb_t* trb, xhci_trb_t* event);
extern xhci_ring_t* __xhci_ring_for(uint8_t slot, uint8_t dci);
extern int __xhci_bulk_out(usb_disk_t* disk, void* buffer, uint32_t len);
extern int __xhci_bulk_in(usb_disk_t* disk, void* buffer, uint32_t len);
extern int __scsi_rw10(usb_disk_t* disk, uint64_t lba, uint16_t blocks, void* buffer, uint8_t write);

int __usb_get_descriptor(uint8_t slot, uint8_t type, uint8_t index, void* buffer, uint16_t len);
int __usb_configure(usb_disk_t* disk);

static uint32_t usb_bot_tag = 0;


/*
Walks a port up to a working disk and names the step it stopped on.
*/
static const char* __usb_enumerate_stage(uint8_t port, uint8_t* out_slot, list_t* usb_devices)
{
    usb_device_descriptor_t desc = {};
    uint8_t slot = 0;

    /* 100ms, the power has to settle */
    tsc_wait_100ns(1000000);

    /* holds PR for 50ms on its own */
    if(__xhci_port_reset(port) != 0)
    {
        return "port reset";
    }

    /* 10ms, TRSTRCY, before we talk */
    tsc_wait_100ns(100000);

    /* no SET_ADDRESS by hand on an xhci : enable slot then address device,
        two commands on the command ring, and the controller sends it. */
    if(__xhci_enable_slot(&slot) != 0)
    {
        return "enable slot";
    }

    *out_slot = slot;

    if(__xhci_address_device(slot, port) != 0)
    {
        return "address device";
    }

    /* eight bytes first! bMaxPacketSize0 is at offset 7, and until we have
        read it we do not know how much we are allowed to ask for. */
    if(__usb_get_descriptor(slot, USB_DESC_DEVICE, 0, &desc, 8) != 0)
    {
        return "first 8 descriptor bytes";
    }

    kernel_debug_output(KDB_LVL_INFO, "usb : port %d, max packet %d", port, desc.max_packet_size0);

    /* a superspeed device answers 9 and means 2^9,
       everything below it answers the byte count itself : 8, 16, 32 or 64. */
    uint16_t max_packet = (desc.max_packet_size0 == 9) ? 512 : desc.max_packet_size0;

    /* and the controller has to be told before anything longer than the
        eight bytes above is asked for. */
    if(__xhci_evaluate_context(slot, max_packet) != 0)
    {
        return "evaluate context";
    }

    if(__usb_get_descriptor(slot, USB_DESC_DEVICE, 0, &desc, sizeof(desc)) != 0)
    {
        return "full descriptor";
    }


    usb_disk_t* disk = vmalloc(sizeof(usb_disk_t));

    disk->slot = slot;
    disk->port = port;
    disk->lun = 0;
    disk->controller = xhci_current();

    if(__usb_configure(disk) != 0)
    {
        vmfree(disk);
        return "configure";
    }

    if(__scsi_read_capacity(disk) != 0)
    {
        vmfree(disk);
        return "read capacity";
    }

    list_insert(usb_devices, port, disk);

    return 0;
}

static int __usb_bot_clear_stall(usb_disk_t* disk, uint8_t dir)
{
    uint8_t dci = (dir == USB_CBW_DIR_IN) ? disk->bulk_in_dci : disk->bulk_out_dci;
    usb_setup_packet_t setup = {};
    xhci_trb_t trb = {};
    xhci_trb_t event = {};

    /* 1. tell the device */
    setup.request_type = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT;
    setup.request = USB_REQ_CLEAR_FEATURE;
    setup.value = USB_FEATURE_ENDPOINT_HALT;
    setup.index = (dir == USB_CBW_DIR_IN) ? disk->bulk_in_ep : disk->bulk_out_ep;

    __xhci_control_transfer(disk->slot, &setup, 0, 0);

    /* 2. take the endpoint out of Halted on the controller side */
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_RESET_EP)
            | ((uint32_t) disk->slot << 24) | ((uint32_t) dci << 16);
    __xhci_command(&trb, &event);

    /* 3. and say where to resume. without this the controller picks up on
       the trb that failed and the stall repeats forever. */
    xhci_ring_t* ring = __xhci_ring_for(disk->slot, dci);

    trb.parameter = (ring->physical + ring->enqueue * XHCI_TRB_SIZE) | ring->cycle;
    trb.status = 0;
    trb.control = XHCI_TRB_TYPE(XHCI_TRB_SET_TR_DEQUEUE)
            | ((uint32_t) disk->slot << 24) | ((uint32_t) dci << 16);

    return (__xhci_command(&trb, &event) == XHCI_COMP_SUCCESS) ? 0 : -1;
}

static int __usb_bot_check_csw(usb_csw_t* csw, uint32_t tag, uint32_t asked)
{
    if(csw->signature != USB_CSW_SIGNATURE || csw->tag != tag)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : csw out of sync, sig 0%x tag %d for %d",
                csw->signature, csw->tag, tag);
        return -1;
    }

    if(csw->status == USB_CSW_STATUS_PHASE_ERROR)
    {
        /* the device lost track of where it is. only a full reset gets it
           back, and on an xhci that is three more commands, see below. */
        return -1;
    }

    if(csw->status != USB_CSW_STATUS_PASS)
    {
        /* it answered, it just refused. REQUEST_SENSE says why, and asking
           is what turns "the read failed" into something readable. */
        kernel_debug_output(KDB_LVL_ERROR, "usb : command refused, %d of %d bytes short, status %d",
                csw->residue, asked, csw->status);
        return -1;
    }

    /* short reads are not an error by themselves, but a caller that asked
       for a sector and got half of one must not use the other half. */
    if(csw->residue)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : short transfer, %d of %d missing", csw->residue, asked);
        return -1;
    }

    return 0;
}

/*
Turns a completion code into the 0 or -1 the callers here test for.
*/
int __usb_transfer_result(int code)
{
    return (code == XHCI_COMP_SUCCESS || code == XHCI_COMP_SHORT_PACKET) ? 0 : -1;
}

int __usb_get_descriptor(uint8_t slot, uint8_t type, uint8_t index,
        void* out, uint16_t len)
{
    usb_setup_packet_t setup = {};

    setup.request_type = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE;
    setup.request = USB_REQ_GET_DESCRIPTOR;

    /* the type goes in the high byte of wValue, the index in the low one.
       putting them the other way round asks for descriptor 6 of type 0. */
    setup.value = ((uint16_t) type << 8) | index;
    setup.index = 0;
    setup.length = len;

    return __usb_transfer_result(__xhci_control_transfer(slot, &setup, out, len));
}

/*
Reads the configuration blob, finds the bulk only mass storage interface and
keeps its two bulk endpoints. The blob is read twice : nine bytes to learn
total_length, then the whole thing.
*/
int __usb_configure(usb_disk_t* disk)
{
    uint8_t buffer[256] = {};
    usb_configuration_descriptor_t* cfg = (usb_configuration_descriptor_t*) buffer;

    if(__usb_get_descriptor(disk->slot, USB_DESC_CONFIGURATION, 0, buffer, 9) != 0)
    {
        return -1;
    }

    uint16_t total = cfg->total_length;

    if(total > sizeof(buffer))
    {
        total = sizeof(buffer);
    }

    if(__usb_get_descriptor(disk->slot, USB_DESC_CONFIGURATION, 0, buffer, total) != 0)
    {
        return -1;
    }

    uint8_t found = 0;
    uint16_t offset = 0;

    while(offset + 2 <= total)
    {
        uint8_t length = buffer[offset];
        uint8_t type = buffer[offset + 1];

        if(!length)
        {
            break;
        }

        if(type == USB_DESC_INTERFACE)
        {
            usb_interface_descriptor_t* itf = (usb_interface_descriptor_t*)(buffer + offset);

            found = (itf->interface_class == USB_CLASS_MASS_STORAGE
                    && itf->interface_subclass == USB_SUBCLASS_SCSI
                    && itf->interface_protocol == USB_PROTOCOL_BULK_ONLY);
        }
        else if(type == USB_DESC_ENDPOINT && found)
        {
            usb_endpoint_descriptor_t* ep = (usb_endpoint_descriptor_t*)(buffer + offset);

            if(USB_EP_XFER_TYPE(ep->attributes) == USB_XFER_BULK)
            {
                if(USB_EP_IS_IN(ep->address))
                {
                    disk->bulk_in_ep = USB_EP_NUMBER(ep->address);
                    disk->bulk_in_dci = XHCI_DCI(disk->bulk_in_ep, 1);
                }
                else
                {
                    disk->bulk_out_ep = USB_EP_NUMBER(ep->address);
                    disk->bulk_out_dci = XHCI_DCI(disk->bulk_out_ep, 0);
                }
            }
        }

        offset += length;
    }

    if(!disk->bulk_in_dci || !disk->bulk_out_dci)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : no bulk only mass storage interface");
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "usb : bulk in ep %d dci %d, bulk out ep %d dci %d",
            disk->bulk_in_ep, disk->bulk_in_dci, disk->bulk_out_ep, disk->bulk_out_dci);

    if(__xhci_configure_endpoints(disk) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : slot %d, configure endpoints failed", disk->slot);
        return -1;
    }

    usb_setup_packet_t setup = {};

    setup.request_type = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIPIENT_DEVICE;
    setup.request = USB_REQ_SET_CONFIGURATION;
    setup.value = cfg->configuration_value;

    return __usb_transfer_result(__xhci_control_transfer(disk->slot, &setup, 0, 0));
}

int __usb_enumerate(uint8_t port, list_t* usb_devices)
{
    uint8_t slot = 0;
    const char* stage = __usb_enumerate_stage(port, &slot, usb_devices);

    if(stage)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : port %d slot %d stopped at %s", port, slot, stage);
        return -1;
    }

    return 0;
}

int __usb_bot_command(usb_disk_t* disk, uint8_t* cdb, uint8_t cdb_len,
        void* buffer, uint32_t len, uint8_t dir)     /* usb_disk_t : lot 6 */
{
    usb_cbw_t cbw = {};
    usb_csw_t csw = {};

    xhci_select(disk->controller);

    cbw.signature = USB_CBW_SIGNATURE;
    cbw.tag = ++usb_bot_tag;
    cbw.transfer_length = len;
    cbw.flags = dir;
    cbw.lun = 0;
    cbw.command_length = cdb_len;
    memcpy(cbw.command, cdb, cdb_len);

    /* 31 and not sizeof(cbw)! the struct is packed at 31 bytes today, but a
       device that gets 32 answers a phase error and there is nothing in the
       log to say why. */
    if(__xhci_bulk_out(disk, &cbw, USB_CBW_SIZE) != 0)
    {
        return -1;
    }

    if(len)
    {
        int ret = (dir == USB_CBW_DIR_IN) ? __xhci_bulk_in(disk, buffer, len)
                                          : __xhci_bulk_out(disk, buffer, len);

        /* a stall on the data phase is not fatal : the device still owes us
           a csw, and it is the csw that says what went wrong. */
        if(ret != 0)
        {
            __usb_bot_clear_stall(disk, dir);
        }
    }

    if(__xhci_bulk_in(disk, &csw, USB_CSW_SIZE) != 0)
    {
        return -1;
    }

    return __usb_bot_check_csw(&csw, cbw.tag, len);
}

/*
ext2.c addresses its partition from zero, so the offset lives here. Everything
above this line thinks the stick starts at the superblock.
*/
int __usb_read(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    usb_disk_t* disk = device->drive;

    /* same units as the AHCI driver : 512 byte sectors, so ext2.c cannot
        tell the difference. convert here if the device uses 4096. */
    while (len)
    {
        uint64_t count = (len * ATA_SECTOR_SIZE) > USB_BOUNCE_SIZE ? USB_BOUNCE_SIZE / disk->block_size : len;


        if(__scsi_rw10(disk, get_first_lba(device) + lba, count, buffer, USB_READ) != 0)
        {
            return -1;
        }

        buffer += count * ATA_SECTOR_SIZE;
        lba += count;
        len -= count;
    }

    return 0;
}

int __usb_write(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    usb_disk_t* disk = device->drive;

    while (len)
    {
        uint64_t count = (len * ATA_SECTOR_SIZE) > USB_BOUNCE_SIZE ? USB_BOUNCE_SIZE / disk->block_size : len;


        if(__scsi_rw10(disk, get_first_lba(device) + lba, count, buffer, USB_WRITE) != 0)
        {
            return -1;
        }

        buffer += USB_BOUNCE_SIZE;
        lba += count;
        len -= count;
    }

    return 0;
}
