#pragma once

/*
Source : https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_type_GUIDs

PARTITION TYPE GUIDS

GUID                                    Description
00000000-0000-0000-0000-000000000000    Unused entry
C12A7328-F81F-11D2-BA4B-00A0C93EC93B    EFI System partition
0FC63DAF-8483-4772-8E79-3D69D8477DE4    Linux filesystem data
4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709    Root partition (x86-64)
BC13C2FF-59E6-4262-A352-B275FD6F7172    /boot partition
0657FD6D-A4AB-43C4-84E5-0933C84B4F4F    Swap partition

BALROG PARTITION TYPE GUIDS

GUID                                    Description
42414C52-0002-4F53-BA10-1B2619BD897F    BalrogOS filesystem data
42414C52-0001-4F53-BA10-1B2619BD897F    BalrogOS root partition (x86-64)
42414C52-0003-4F53-BA10-1B2619BD897F    BalrogOS /boot partition
42414C52-0004-4F53-BA10-1B2619BD897F    BalrogOS swap partition
*/

/*
C12A7328-F81F-11D2-BA4B-00A0C93EC93B, efi system partition mount point /boot/efi/EFI
-> fat32 -> unreadable atm
TODO :  fat32
*/
#define GPT_TYPE_EFI_SYSTEM                     \
{                                               \
    /*  uint32_t (reverse little endian)  */    \
    0x28, 0x73, 0x2a, 0xc1,                     \
    /*  uint16_t (reverse little endian)  */    \
    0x1f, 0xf8,                                 \
    /*  uint16_t (reverse little endian)  */    \
    0xd2, 0x11,                                 \
    /*  DATA byte table  */                     \
    0xba, 0x4b,                                 \
    0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b          \
}

/*
42414C52-0002-4F53-BA10-1B2619BD897F, BalrogOS filesystem data
*/
#define GPT_TYPE_BALROG_FS                      \
{                                               \
    /*  uint32_t (reverse little endian)  */    \
    0x52, 0x4c, 0x41, 0x42,                     \
    /*  uint16_t (reverse little endian)  */    \
    0x02, 0x00,                                 \
    /*  uint16_t (reverse little endian)  */    \
    0x53, 0x4f,                                 \
    /*  DATA byte table  */                     \
    0xba, 0x10,                                 \
    0x1b, 0x26, 0x19, 0xbd, 0x89, 0x7f          \
}

/*
42414C52-0001-4F53-BA10-1B2619BD897F, BalrogOS root partition (x86-64)
*/
#define GPT_TYPE_BALROG_ROOT                    \
{                                               \
    /*  uint32_t (reverse little endian)  */    \
    0x52, 0x4c, 0x41, 0x42,                     \
    /*  uint16_t (reverse little endian)  */    \
    0x01, 0x00,                                 \
    /*  uint16_t (reverse little endian)  */    \
    0x53, 0x4f,                                 \
    /*  DATA byte table  */                     \
    0xba, 0x10,                                 \
    0x1b, 0x26, 0x19, 0xbd, 0x89, 0x7f          \
}

/*
42414C52-0003-4F53-BA10-1B2619BD897F, BalrogOS /boot partition
*/
#define GPT_TYPE_BALROG_BOOT                    \
{                                               \
    /*  uint32_t (reverse little endian)  */    \
    0x52, 0x4c, 0x41, 0x42,                     \
    /*  uint16_t (reverse little endian)  */    \
    0x03, 0x00,                                 \
    /*  uint16_t (reverse little endian)  */    \
    0x53, 0x4f,                                 \
    /*  DATA byte table  */                     \
    0xba, 0x10,                                 \
    0x1b, 0x26, 0x19, 0xbd, 0x89, 0x7f          \
}

/*
42414C52-0004-4F53-BA10-1B2619BD897F, BalrogOS swap partition
*/
#define GPT_TYPE_BALROG_SWAP                    \
{                                               \
    /*  uint32_t (reverse little endian)  */    \
    0x52, 0x4c, 0x41, 0x42,                     \
    /*  uint16_t (reverse little endian)  */    \
    0x04, 0x00,                                 \
    /*  uint16_t (reverse little endian)  */    \
    0x53, 0x4f,                                 \
    /*  DATA byte table  */                     \
    0xba, 0x10,                                 \
    0x1b, 0x26, 0x19, 0xbd, 0x89, 0x7f          \
}
