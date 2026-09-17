#pragma once

#include <stdint.h>

/*
Bulk-Only Transport, and the few scsi commands a usb key actually answers.

References :
mass storage  : https://wiki.osdev.org/USB_Mass_Storage_Class_Devices
bulk only 1.0 : https://www.usb.org/document-library/mass-storage-bulk-only-10
scsi opcodes  : https://www.t10.org/lists/op-num.htm
*/

/*
The two signatures, as bytes : "USBC" going out, "USBS" coming back. A status
wrapper with the wrong signature or the wrong tag means the two ends are out of
step so reset the endpoint, don't keep reading data that slid by one transfer.
*/
#define USB_CBW_SIGNATURE       0x43425355  // "USBC"
#define USB_CSW_SIGNATURE       0x53425355  // "USBS"

#define USB_CBW_SIZE            31
#define USB_CSW_SIZE            13

#define USB_CBW_DIR_OUT         0x00    // host to device
#define USB_CBW_DIR_IN          0x80    // device to host

/*
The command block wrapper, 31 bytes out on the bulk out endpoint.

We pick the tag and the device hands it back untouched in the status wrapper.
That's the only way to know whose answer we're reading.
*/
typedef struct __usb_cbw_t
{
    uint32_t signature;         // 0x00, USB_CBW_SIGNATURE
    uint32_t tag;               // 0x04, echoed back in the csw
    uint32_t transfer_length;   // 0x08, bytes the data phase will carry
    uint8_t flags;              // 0x0C, USB_CBW_DIR_IN or USB_CBW_DIR_OUT
    uint8_t lun;                // 0x0D, logical unit, 0 on every stick we care about
    uint8_t command_length;     // 0x0E, how many bytes of command below are used
    uint8_t command[16];        // 0x0F, the scsi command block itself
} __attribute__((packed)) usb_cbw_t;

/*
The command status wrapper, 13 bytes back on the bulk in endpoint.

residue is what didn't get transferred. A phase error means the device lost
track of where it is, and getting out of that takes a Bulk-Only Mass Storage
Reset, then CLEAR_FEATURE(ENDPOINT_HALT) on both endpoints, then on an xhci a
Reset Endpoint and a Set TR Dequeue Pointer on top of all that.
*/
typedef struct __usb_csw_t
{
    uint32_t signature;         // 0x00, USB_CSW_SIGNATURE
    uint32_t tag;               // 0x04, the tag of the cbw it answers
    uint32_t residue;           // 0x08, bytes not transferred
    uint8_t status;             // 0x0C, one of USB_CSW_STATUS_*
} __attribute__((packed)) usb_csw_t;

#define USB_CSW_STATUS_PASS         0x00
#define USB_CSW_STATUS_FAIL         0x01
#define USB_CSW_STATUS_PHASE_ERROR  0x02

/*
Class specific requests, sent on the control endpoint and not on the bulk pair.
*/
#define USB_MSC_REQ_RESET       0xFF    // bulk only mass storage reset
#define USB_MSC_REQ_GET_MAX_LUN 0xFE    // how many logical units, 0 based

#define USB_READ                0
#define USB_WRITE               1

/*
Six commands is all it takes to read and write a key.
*/
#define SCSI_TEST_UNIT_READY    0x00    // 6 bytes, does it answer
#define SCSI_REQUEST_SENSE      0x03    // 6 bytes, why the last one failed
#define SCSI_INQUIRY            0x12    // 6 bytes, device type and vendor
#define SCSI_READ_CAPACITY_10   0x25    // 10 bytes, block count and block size
#define SCSI_READ_10            0x28    // 10 bytes
#define SCSI_WRITE_10           0x2A    // 10 bytes

/*
TEST UNIT READY fails on the first try more often than not, and that's normal,
the device just isn't awake yet. Do a REQUEST SENSE and retry a few times with
a delay. Skip that and you get a driver that works only sometime
*/
#define SCSI_READY_RETRIES      5

/*
READ(10) and WRITE(10) have the same layout, only the opcode changes.

lba and count are big endian, the one place in this whole kernel where that
happens. The struct is here for the shape, but those two get filled byte by
byte :

    cdb->lba_be[0] = (lba >> 24) & 0xFF;
    cdb->lba_be[1] = (lba >> 16) & 0xFF;
    cdb->lba_be[2] = (lba >> 8) & 0xFF;
    cdb->lba_be[3] = lba & 0xFF;
*/
typedef struct __scsi_read_write_10_t
{
    uint8_t opcode;         // 0x00, SCSI_READ_10 or SCSI_WRITE_10
    uint8_t flags;          // 0x01, 0 does what we want
    uint8_t lba_be[4];      // 0x02, big endian
    uint8_t reserved;       // 0x06
    uint8_t count_be[2];    // 0x07, block count, big endian
    uint8_t control;        // 0x09, 0
} __attribute__((packed)) scsi_read_write_10_t;

/*
What READ CAPACITY(10) answers. Eight bytes, both big endian.
*/
typedef struct __scsi_capacity_10_t
{
    uint8_t last_lba_be[4];     // 0x00, the last addressable block, not the count
    uint8_t block_size_be[4];   // 0x04, bytes per block
} __attribute__((packed)) scsi_capacity_10_t;

/*
What INQUIRY answers. We only ask for the first 36 bytes and only the three
text fields are useful, they're what the boot log prints to say which key
answered.

None of them is null terminated, they're space padded -> copy and terminate,
same treatment the ahci model string needed after it printed garbage.
*/
typedef struct __scsi_inquiry_t
{
    uint8_t device_type;        // 0x00, 0 is a direct access block device
    uint8_t removable;          // 0x01, bit 7 set on a usb key
    uint8_t version;            // 0x02
    uint8_t response_format;    // 0x03
    uint8_t additional_length;  // 0x04, bytes that follow, 31 for a 36 byte answer
    uint8_t reserved[3];        // 0x05
    char vendor[8];             // 0x08, space padded, no terminator
    char product[16];           // 0x10, space padded, no terminator
    char revision[4];           // 0x20, space padded, no terminator
} __attribute__((packed)) scsi_inquiry_t;

#define SCSI_INQUIRY_SIZE       36

/*
What REQUEST SENSE answers, fixed format.
*/
typedef struct __scsi_sense_t
{
    uint8_t response_code;      // 0x00, 0x70 or 0x71 in the fixed format
    uint8_t reserved0;          // 0x01
    uint8_t sense_key;          // 0x02, the coarse reason, in the low 4 bits
    uint8_t information[4];     // 0x03
    uint8_t additional_length;  // 0x07
    uint8_t reserved1[4];       // 0x08
    uint8_t asc;                // 0x0C, additional sense code
    uint8_t ascq;               // 0x0D, and its qualifier
    uint8_t reserved2[4];       // 0x0E
} __attribute__((packed)) scsi_sense_t;

#define SCSI_SENSE_SIZE         18

#define SCSI_SENSE_KEY(s)       ((s) & 0x0F)

#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_NOT_READY        0x02    // what a key answers while it wakes up
#define SCSI_SENSE_MEDIUM_ERROR     0x03
#define SCSI_SENSE_HARDWARE_ERROR   0x04
#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
#define SCSI_SENSE_UNIT_ATTENTION   0x06    // the medium changed, retry once

typedef struct __usb_disk_t
{
    uint8_t slot;               // what Enable Slot handed us, 1 based
    uint8_t port;               // the root port it came up on, 0 based here
    uint8_t lun;                // logical unit, 0 on every stick we care about

    /*  the endpoint addresses come from the interface descriptor, the dcis are
        what the controller indexes contexts and doorbells with. they are not
        the same number : dci is (ep * 2) + direction, XHCI_DCI().  */
    uint8_t bulk_in_ep;
    uint8_t bulk_out_ep;
    uint8_t bulk_in_dci;
    uint8_t bulk_out_dci;

    /*  READ_CAPACITY(10) answers these two, and nothing can be read before it
        has. block_size is not always 512 : ext2.c asks in 512 byte sectors,
        so the conversion happens in usb_read.  */
    uint32_t block_size;
    uint32_t last_block;        // the LAST valid block, not a count

    void* controller;
} __attribute__((packed)) usb_disk_t;
