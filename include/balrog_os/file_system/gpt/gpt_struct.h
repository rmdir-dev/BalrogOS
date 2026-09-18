#pragma once

#include <stdint.h>

/*
reference : https://wiki.osdev.org/GPT
the spec  : uefi 2.10, section 5.3
*/

/*  "EFI PART", the eight bytes at lba 1. compared as bytes and not as a
    uint64_t : the constant would have to be written backwards to match.  */
#define GPT_SIGNATURE       "EFI PART"
#define GPT_SIGNATURE_LEN   8

#define GPT_HEADER_LBA      1           // the primary header, the backup one is the last lba
#define GPT_GUID_LEN        16          // a guid is 16 bytes, everywhere in the format

/*  36 characters and the terminator, the text form  */
#define GPT_GUID_TEXT_LEN   37

/*  36 utf-16 characters, one byte each once we drop the high half  */
#define GPT_NAME_LEN        37

/*  a table can declare more, and one that does is not one we made. 128 entries
    of 128 bytes is what sfdisk writes and what the spec asks for as a minimum,
    and it caps the read at 32 sectors.  */
#define GPT_MAX_ENTRIES     128

/*
PARTITION TABLE HEADER, LBA 1

Offset      Length      Contents
0 (0x00)    8 bytes     Signature ("EFI PART", 45h 46h 49h 20h 50h 41h 52h 54h)
8 (0x08)    4 bytes     Revision number of header - 1.0 (00h 00h 01h 00h) for UEFI 2.10
12 (0x0C)   4 bytes     Header size in little endian (in bytes, usually 5Ch 00h 00h 00h
                        or 92 bytes)
16 (0x10)   4 bytes     CRC-32 of header (offset +0 to +0x5B) in little endian, with this
                        field zeroed during calculation
20 (0x14)   4 bytes     Reserved; must be zero
24 (0x18)   8 bytes     Current LBA (location of this header copy)
32 (0x20)   8 bytes     Backup LBA (location of the other header copy)
40 (0x28)   8 bytes     First usable LBA for partitions (primary partition table last
                        LBA + 1)
48 (0x30)   8 bytes     Last usable LBA for partitions (secondary partition table first
                        LBA - 1)
56 (0x38)   16 bytes    Disk GUID in little endian
72 (0x48)   8 bytes     Starting LBA of array of partition entries (usually 2 for
                        compatibility)
80 (0x50)   4 bytes     Number of partition entries in array
84 (0x54)   4 bytes     Size of a single partition entry (usually 80h or 128)
88 (0x58)   4 bytes     CRC-32 of partition entries array in little endian
92 (0x5C)   *           Reserved; must be zeroes for the rest of the block (420 bytes for
                        a sector size of 512 bytes)
*/
typedef struct __gpt_header_t
{
    uint8_t  signature[8];              // 0x00, "EFI PART", 45h 46h 49h 20h 50h 41h 52h 54h
    uint32_t revision;                  // 0x08, revision number of header, 1.0 is 00h 00h 01h 00h
    uint32_t header_size;               // 0x0C, header size in little endian, usually 92
    uint32_t header_crc32;              // 0x10, CRC-32 of header, this field zeroed during calculation
    uint32_t reserved;                  // 0x14, reserved, must be zero
    uint64_t current_lba;               // 0x18, location of this header copy
    uint64_t backup_lba;                // 0x20, location of the other header copy
    uint64_t first_usable_lba;          // 0x28, primary partition table last LBA + 1
    uint64_t last_usable_lba;           // 0x30, secondary partition table first LBA - 1
    uint8_t  disk_guid[GPT_GUID_LEN];   // 0x38, disk GUID in little endian
    uint64_t entry_lba;                 // 0x48, starting LBA of the entry array, usually 2
    uint32_t entry_count;               // 0x50, number of partition entries in array
    uint32_t entry_size;                // 0x54, size of a single partition entry, usually 128
    uint32_t entry_crc32;               // 0x58, CRC-32 of partition entries array
    /*  0x5C and up is "reserved, must be zeroes for the rest of the block",
        420 bytes for a sector size of 512. Not declared : the structure is
        memcpy'd out of a sector buffer, so what follows it is never read.
    */
} __attribute__((packed)) gpt_header_t;

/*
GUID PARTITION ENTRY, LBA 2 TO 33

Offset      Length      Contents
0 (0x00)    16 bytes    Partition type GUID (little endian)
16 (0x10)   16 bytes    Unique partition GUID (little endian)
32 (0x20)   8 bytes     First LBA (little endian)
40 (0x28)   8 bytes     Last LBA (inclusive, usually odd)
48 (0x30)   8 bytes     Attribute flags (e.g. bit 60 denotes read-only)
56 (0x38)   72 bytes    Partition name (36 UTF-16LE code units)

ATTRIBUTE FLAGS

Bit         Content
0           Platform required (required by the computer to function properly, OEM
            partition for example)
1           EFI firmware should ignore the content of the partition and not try to read
            from it
2           Legacy BIOS bootable (equivalent to active flag at offset +0h in MBR
            partition entries)
3-47        Reserved for future use
48-63       Defined and used by the individual partition type
*/
typedef struct __gpt_entry_t
{
    uint8_t  type_guid[GPT_GUID_LEN];   // 0x00, partition type GUID, little endian
    uint8_t  unique_guid[GPT_GUID_LEN]; // 0x10, unique partition GUID, little endian
    uint64_t first_lba;                 // 0x20, first LBA, little endian
    /*  the LAST valid lba, not a count. the size is last - first + 1, the same
        off by one usb_disk_t.last_block carries.
    */
    uint64_t last_lba;                  // 0x28, last LBA, inclusive, usually odd
    uint64_t attributes;                // 0x30, attribute flags, bit 60 denotes read-only
    /*  36 utf-16le code units. not a c string, and not terminated when it is
        full.
    */
    uint16_t name[36];                  // 0x38, partition name
} __attribute__((packed)) gpt_entry_t;

typedef struct __gpt_partition_t
{
    uint8_t  type_guid[GPT_GUID_LEN];   // what the partition is for
    uint8_t  unique_guid[GPT_GUID_LEN]; // which partition it is, the one we match on
    uint64_t first_lba;                 // where the volume starts, what root_lba becomes
    uint64_t last_lba;                  // inclusive, so the size is last - first + 1
    char     name[GPT_NAME_LEN];        // the utf-16le name folded to ascii
    /*  which slot of the table it came from, 0 based. the partition number an
        operator sees is this plus one.
    */
    uint32_t index;
} gpt_partition_t;