#pragma once

#include <stdint.h>

/*
The usb protocol itself, one layer above the controller.

Everything below is little endian, unlike the scsi blocks in usb_storage.h.

References :
usb          : https://wiki.osdev.org/Universal_Serial_Bus
usb 2.0 spec : chapter 9, Device Framework
timings      : usb 2.0 spec, chapter 7.1.7
*/

/*
The eight bytes of any control request. On an xhci this never goes in a buffer,
the setup stage trb carries it inline in its parameter field with IDT set. Eight
bytes, 64 bits, it fits exactly.
*/
typedef struct __usb_setup_packet_t
{
    uint8_t request_type;   // 0x00, direction, type and recipient
    uint8_t request;        // 0x01, the request code
    uint16_t value;         // 0x02, meaning depends on the request
    uint16_t index;         // 0x04, interface or endpoint, usually
    uint16_t length;        // 0x06, how many bytes the data phase carries
} __attribute__((packed)) usb_setup_packet_t;

/*
The request type byte. Top bit is the direction, and it's the direction of the
data phase, not of the setup packet.
*/
#define USB_DIR_OUT             0x00    // host to device
#define USB_DIR_IN              0x80    // device to host

#define USB_TYPE_STANDARD       0x00
#define USB_TYPE_CLASS          0x20
#define USB_TYPE_VENDOR         0x40

#define USB_RECIPIENT_DEVICE    0x00
#define USB_RECIPIENT_INTERFACE 0x01
#define USB_RECIPIENT_ENDPOINT  0x02

/*
Standard requests. Three of them are enough to bring a usb key up, and
SET_ADDRESS isn't one : on an xhci the Address Device command sends it for us.
*/
#define USB_REQ_GET_STATUS          0
#define USB_REQ_CLEAR_FEATURE       1
#define USB_REQ_SET_FEATURE         3
#define USB_REQ_SET_ADDRESS         5
#define USB_REQ_GET_DESCRIPTOR      6
#define USB_REQ_SET_DESCRIPTOR      7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE       10
#define USB_REQ_SET_INTERFACE       11

/*
Feature selector of CLEAR_FEATURE, to unstick a halted endpoint. On an xhci
clearing it on the device isn't enough! The controller keeps its own halted
state, so Reset Endpoint and Set TR Dequeue Pointer have to follow or the same
error comes straight back on the next transfer.
*/
#define USB_FEATURE_ENDPOINT_HALT   0

#define USB_DESC_DEVICE         1
#define USB_DESC_CONFIGURATION  2
#define USB_DESC_STRING         3
#define USB_DESC_INTERFACE      4
#define USB_DESC_ENDPOINT       5

/*
wValue of a GET_DESCRIPTOR : type in the high byte, index in the low one.
*/
#define USB_DESC_VALUE(type, index) (((type) << 8) | (index))

/*
Every descriptor starts with these two, which is how we walk a configuration
blob without knowing what's in it beforehand.
*/
typedef struct __usb_descriptor_header_t
{
    uint8_t length;         // 0x00, this descriptor, header included
    uint8_t type;           // 0x01, one of USB_DESC_*
} __attribute__((packed)) usb_descriptor_header_t;

/*
The device descriptor, 18 bytes.

Read it twice. The first read asks for 8 bytes only : max_packet_size0 is at
offset 7 and there's no way to know how much the control endpoint takes before
having read it.
*/
typedef struct __usb_device_descriptor_t
{
    uint8_t length;             // 0x00, 18
    uint8_t type;               // 0x01, USB_DESC_DEVICE
    uint16_t usb_version;       // 0x02, bcd, 0x0200 for usb 2.0
    uint8_t device_class;       // 0x04, 0 when the class is on the interface
    uint8_t device_subclass;    // 0x05
    uint8_t device_protocol;    // 0x06
    uint8_t max_packet_size0;   // 0x07, control endpoint, the reason for the first read
    uint16_t vendor_id;         // 0x08
    uint16_t product_id;        // 0x0A
    uint16_t device_version;    // 0x0C, bcd
    uint8_t manufacturer_index; // 0x0E, string index, 0 when there is none
    uint8_t product_index;      // 0x0F
    uint8_t serial_index;       // 0x10
    uint8_t configuration_count;// 0x11
} __attribute__((packed)) usb_device_descriptor_t;

/*
The configuration descriptor, 9 bytes, never read on its own.

GET_DESCRIPTOR(CONFIGURATION) hands back the configuration and every interface
and endpoint behind it in one go, and total_length says how big that is. So
twice again : nine bytes to learn total_length, then that many.
*/
typedef struct __usb_configuration_descriptor_t
{
    uint8_t length;             // 0x00, 9
    uint8_t type;               // 0x01, USB_DESC_CONFIGURATION
    uint16_t total_length;      // 0x02, the whole blob, interfaces included
    uint8_t interface_count;    // 0x04
    uint8_t configuration_value;// 0x05, what SET_CONFIGURATION takes
    uint8_t configuration_index;// 0x06, string index
    uint8_t attributes;         // 0x07, self powered, remote wakeup
    uint8_t max_power;          // 0x08, in 2mA units
} __attribute__((packed)) usb_configuration_descriptor_t;

/*
The interface descriptor. Its three class bytes are what tells a usb key from a
keyboard. They're on the interface and not on the device because one device can
expose several.
*/
typedef struct __usb_interface_descriptor_t
{
    uint8_t length;             // 0x00, 9
    uint8_t type;               // 0x01, USB_DESC_INTERFACE
    uint8_t interface_number;   // 0x02
    uint8_t alternate_setting;  // 0x03
    uint8_t endpoint_count;     // 0x04, endpoint 0 not counted
    uint8_t interface_class;    // 0x05
    uint8_t interface_subclass; // 0x06
    uint8_t interface_protocol; // 0x07
    uint8_t interface_index;    // 0x08, string index
} __attribute__((packed)) usb_interface_descriptor_t;

/*
The endpoint descriptor. Direction is the top bit of the address, transfer type
is the low two bits of the attributes.
*/
typedef struct __usb_endpoint_descriptor_t
{
    uint8_t length;             // 0x00, 7
    uint8_t type;               // 0x01, USB_DESC_ENDPOINT
    uint8_t address;            // 0x02, number in 3:0, direction in bit 7
    uint8_t attributes;         // 0x03, transfer type in 1:0
    uint16_t max_packet_size;   // 0x04
    uint8_t interval;           // 0x06, only meaningful for interrupt and isochronous
} __attribute__((packed)) usb_endpoint_descriptor_t;

#define USB_EP_NUMBER(a)        ((a) & 0x0F)
#define USB_EP_IS_IN(a)         (((a) & 0x80) != 0)

#define USB_XFER_CONTROL        0
#define USB_XFER_ISOCHRONOUS    1
#define USB_XFER_BULK           2
#define USB_XFER_INTERRUPT      3

#define USB_EP_XFER_TYPE(a)     ((a) & 0x03)

/*
The only combination we handle, and every modern stick reports exactly these.
*/
#define USB_CLASS_MASS_STORAGE      0x08
#define USB_SUBCLASS_SCSI           0x06    // transparent scsi command set
#define USB_PROTOCOL_BULK_ONLY      0x50    // bulk only transport

/*
Enumeration delays, in the 100ns units tsc_wait_100ns takes.
*/
#define USB_DELAY_POWER_STABLE  1000000     // 100ms, TATTDB
#define USB_DELAY_PORT_RESET    500000      // 50ms, TDRSTR
#define USB_DELAY_RESET_RECOVER 100000      // 10ms, TRSTRCY
