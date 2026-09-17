#pragma once

#include <stdint.h>
#include "balrog_os/memory/memory.h"

/*
What the xhci controller reads out of our memory.

References :
xHCI      : https://wiki.osdev.org/EXtensible_Host_Controller_Interface
xHCI spec : chapter 6, Data Structures
*/

/*
256 trbs of 16 bytes = one page = one pmm_calloc. The last one is the link trb
so only 255 are usable.
*/
#define XHCI_TRB_SIZE           16
#define XHCI_RING_TRBS          (PAGE_SIZE / XHCI_TRB_SIZE)
#define XHCI_RING_USABLE        (XHCI_RING_TRBS - 1)

/*
trb types, bits 15:10 of the control word.
*/
#define XHCI_TRB_NORMAL         1   // bulk data
#define XHCI_TRB_SETUP_STAGE    2   // the 8 byte setup packet, carried inline
#define XHCI_TRB_DATA_STAGE     3   // the data phase of a control transfer
#define XHCI_TRB_STATUS_STAGE   4   // the handshake that ends a control transfer
#define XHCI_TRB_LINK           6   // last entry of a ring, points back to its head
#define XHCI_TRB_ENABLE_SLOT    9   // ask for a device slot
#define XHCI_TRB_DISABLE_SLOT   10  // give it back
#define XHCI_TRB_ADDRESS_DEVICE 11  // this is what sends SET_ADDRESS for us
#define XHCI_TRB_CONFIG_EP      12  // declare the endpoints of a configuration
#define XHCI_TRB_EVAL_CONTEXT   13  // change a field of a context already in use
#define XHCI_TRB_RESET_EP       14  // take an endpoint out of halted after a stall
#define XHCI_TRB_SET_TR_DEQUEUE 16  // say where to resume, mandatory after a reset
#define XHCI_TRB_NOOP_COMMAND   23  // does nothing, and that is the point
#define XHCI_TRB_TRANSFER_EVENT 32  // written by the controller
#define XHCI_TRB_COMMAND_EVENT  33  // written by the controller
#define XHCI_TRB_PORT_EVENT     34  // written by the controller

/*
The rest of the control word. The type isn't alone in there and two of these
decide whether the driver works or hangs.
*/
#define XHCI_TRB_CYCLE          (1 << 0)    // the current lap, written last
#define XHCI_TRB_ENT            (1 << 1)    // evaluate next trb
#define XHCI_TRB_TC             (1 << 1)    // toggle cycle, link trb only, same bit
#define XHCI_TRB_ISP            (1 << 2)    // interrupt on short packet
#define XHCI_TRB_CHAIN          (1 << 4)    // this trb and the next are one transfer
#define XHCI_TRB_IOC            (1 << 5)    // interrupt on completion
#define XHCI_TRB_IDT            (1 << 6)    // the parameter field holds data, not an address
#define XHCI_TRB_TYPE(t)        ((t) << 10)
#define XHCI_TRB_GET_TYPE(c)    (((c) >> 10) & 0x3F)

/*
IOC is the trap of this whole thing! A trb without it runs fine, completes
fine, and produces no event at all. So we sit there waiting for something that
never comes, and it looks like a timeout on a transfer that actually worked.
Put it on the last trb of every transfer.

TC shares a bit with ENT, they belong to different trb types so it's fine. On a
link trb it tells the controller to flip its cycle when it wraps, and we flip
ours at the same spot. Forget it and the ring still turns, but the controller
replays the previous lap. Took me a while to even suspect that one.
*/

/*
Transfer type of a setup stage trb, bits 17:16. Get it wrong and the request
fails with a perfectly good setup packet.
*/
#define XHCI_TRT_NO_DATA        (0 << 16)
#define XHCI_TRT_OUT_DATA       (2 << 16)
#define XHCI_TRT_IN_DATA        (3 << 16)

/*
Completion codes, bits 31:24 of an event's status word.
*/
#define XHCI_COMP_SUCCESS       1   // nothing to do
#define XHCI_COMP_DATA_BUFFER   2   // over or underrun
#define XHCI_COMP_BABBLE        3   // the device talked longer than it was allowed
#define XHCI_COMP_TRANSACTION   4   // no answer, or a bad one
#define XHCI_COMP_TRB_ERROR     5   // we built the trb wrong
#define XHCI_COMP_STALL         6   // endpoint halted, needs a reset endpoint
#define XHCI_COMP_EP_NOT_ENABLED 12 // the context says the endpoint is not usable
#define XHCI_COMP_SHORT_PACKET  13  // fewer bytes than asked, often not an error at all

#define XHCI_TRB_COMP_CODE(s)   (((s) >> 24) & 0xFF)
#define XHCI_TRB_XFER_LEN(s)    ((s) & 0x1FFFF)     // 17 bits, so 128KiB per trb
#define XHCI_TRB_SLOT_ID(c)     (((c) >> 24) & 0xFF)
#define XHCI_TRB_DIR_IN         (1 << 16)

/*
One trb. Every ring is an array of these, the shape never changes, only what
parameter and status mean depends on the type.

  parameter   an address, or the payload itself when IDT is set
  status      length going out, completion code coming back
  control     cycle bit, flags, type
*/
typedef struct __xhci_trb_t
{
    uint64_t parameter;     // 0x00, buffer address, or 8 bytes of immediate data
    uint32_t status;        // 0x08, length out, completion code and residue in
    uint32_t control;       // 0x0C, cycle, flags, and the type in 15:10
} __attribute__((packed)) xhci_trb_t;

/*
One segment of the event ring. A single entry is enough, we'll never have more
events in flight than a page holds. The event ring is the only one without a
link trb -> this table is what bounds it.
*/
typedef struct __xhci_erst_entry_t
{
    uint64_t ring_base;     // 0x00, physical address of the segment
    uint32_t ring_size;     // 0x08, how many trbs, in the low 16 bits
    uint32_t reserved;      // 0x0C
} __attribute__((packed)) xhci_erst_entry_t;

/*
Endpoint types, field 5:3 of the second dword of an endpoint context. Careful,
this is not the usb numbering : direction and type are packed together, and the
control endpoint is bidirectional so it gets a value of its own.
*/
#define XHCI_EP_NOT_VALID       0
#define XHCI_EP_ISOCH_OUT       1
#define XHCI_EP_BULK_OUT        2
#define XHCI_EP_INTERRUPT_OUT   3
#define XHCI_EP_CONTROL         4
#define XHCI_EP_ISOCH_IN        5
#define XHCI_EP_BULK_IN         6
#define XHCI_EP_INTERRUPT_IN    7

/*
Where an endpoint sits inside a device context. Not the usb endpoint number!
The slot context eats index 0, so the control endpoint is 1 and the rest is
numbered in pairs.

    endpoint 0, control, both ways     dci 1
    endpoint 1 OUT                     dci 2
    endpoint 1 IN                      dci 3
    endpoint 2 OUT                     dci 4

The formula only works from endpoint 1 up. Control is bidirectional, it has no
OUT/IN pair, so it doesn't fit and gets its own constant -> XHCI_DCI(0, 0)
gives 0, which is the slot context, not an endpoint. Be one off here and you
configure the wrong endpoint, and the transfer goes nowhere quietly.
*/
#define XHCI_DCI(ep, in)        (((ep) * 2) + ((in) ? 1 : 0))
#define XHCI_DCI_CONTROL        1
#define XHCI_MAX_ENDPOINTS      31  // 1 to 31, the index 0 being the slot context

/*
The slot context, what the controller knows about a device as a whole. We fill
it in, then it writes its own answers back into the copy it keeps.
*/
typedef struct __xhci_slot_context_t
{
    uint32_t dword0;    // route string, speed, hub flag, context entries
    uint32_t dword1;    // max exit latency, root hub port number, port count
    uint32_t dword2;    // parent hub slot and port, interrupter target
    uint32_t dword3;    // usb device address, slot state
    uint32_t reserved[4];
} __attribute__((packed)) xhci_slot_context_t;

#define XHCI_SLOT_ROUTE(r)          ((r) & 0xFFFFF)
#define XHCI_SLOT_SPEED(s)          (((s) & 0x0F) << 20)
#define XHCI_SLOT_CTX_ENTRIES(n)    (((n) & 0x1F) << 27)   // the highest dci in use
#define XHCI_SLOT_ROOT_PORT(p)      (((p) & 0xFF) << 16)
#define XHCI_SLOT_GET_ADDRESS(d)    ((d) & 0xFF)
#define XHCI_SLOT_GET_STATE(d)      (((d) >> 27) & 0x1F)

/*
One per endpoint the device exposes. dequeue holds the physical address of that
endpoint's transfer ring, and its bit 0 isn't part of the address : it's the
cycle state the controller starts on, and it has to match ours.
*/
typedef struct __xhci_endpoint_context_t
{
    uint32_t dword0;    // endpoint state, interval, max esit payload high
    uint32_t dword1;    // error count, endpoint type, max burst, max packet size
    uint64_t dequeue;   // transfer ring address, plus the dequeue cycle state in bit 0
    uint32_t dword4;    // average trb length, max esit payload low
    uint32_t reserved[3];
} __attribute__((packed)) xhci_endpoint_context_t;

#define XHCI_EP_TYPE(t)             (((t) & 0x07) << 3)
#define XHCI_EP_ERROR_COUNT(n)      (((n) & 0x03) << 1)    // 3 retries is the usual value
#define XHCI_EP_MAX_BURST(n)        (((n) & 0xFF) << 8)
#define XHCI_EP_MAX_PACKET(n)       (((n) & 0xFFFF) << 16)
#define XHCI_EP_AVG_TRB_LEN(n)      ((n) & 0xFFFF)
#define XHCI_EP_DEQUEUE_CYCLE       (1 << 0)
#define XHCI_EP_GET_STATE(d)        ((d) & 0x07)

/*
First block of an input context. Says which of the contexts behind it the
controller should actually look at, one bit per context indexed by dci -> bit 0
is the slot, bit 1 is the control endpoint. Address Device sets those two and
nothing else.
*/
typedef struct __xhci_input_control_context_t
{
    uint32_t drop_flags;    // 0x00, contexts to forget, bits 0 and 1 always zero
    uint32_t add_flags;     // 0x04, contexts to take into account
    uint32_t reserved[6];
} __attribute__((packed)) xhci_input_control_context_t;

#define XHCI_INPUT_SLOT_FLAG        (1 << 0)
#define XHCI_INPUT_EP_FLAG(dci)     (1 << (dci))

/*
A slot context followed by up to 31 endpoint contexts. The controller owns this
one, we just hand it the page. Its physical address goes in the dcbaa, indexed
by slot id.

Both of these are only right while HCCPARAMS1.CSZ is clear! With CSZ set every
context is 64 bytes instead of 32 and everything after the first one shifts.
Read CSZ once at init and index with that, don't sizeof these, or nothing past
the slot context lands where the controller looks for it.
*/
typedef struct __xhci_device_context_t
{
    xhci_slot_context_t slot;
    xhci_endpoint_context_t endpoints[XHCI_MAX_ENDPOINTS];
} __attribute__((packed)) xhci_device_context_t;

/*
What we hand to Address Device and Configure Endpoint. Same as a device context
with the control block bolted in front.
*/
typedef struct __xhci_input_context_t
{
    xhci_input_control_context_t control;
    xhci_slot_context_t slot;
    xhci_endpoint_context_t endpoints[XHCI_MAX_ENDPOINTS];
} __attribute__((packed)) xhci_input_context_t;

/*
A ring plus the bits we have to remember ourselves. Nothing tells us where we
are in a ring : CRCR reads back as zero and there's no register for the enqueue
position, so the index and the cycle state live here or nowhere.
*/
typedef struct __xhci_ring_t
{
    xhci_trb_t* trbs;       // virtual address, P2V of the page
    uintptr_t physical;     // what goes in the registers and the link trb
    uint32_t enqueue;       // index of the next trb we write
    uint32_t dequeue;       // index of the next event we read, event rings only
    uint8_t cycle;          // the lap we are on, flipped at every wrap
} __attribute__((packed)) xhci_ring_t;
