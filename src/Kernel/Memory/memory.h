#pragma once

/*
ADDRESSES
*/
#define KERNEL_OFFSET 0xFFFFFF8000000000

#include <stdint.h>

#define VIRTUAL_TO_PHYSICAL(addr) (uintptr_t)addr & ~KERNEL_OFFSET
#define PHYSICAL_TO_VIRTUAL(addr) (uintptr_t)addr | KERNEL_OFFSET

#define ONE_MiB 1048576
#define ONE_kiB 1024

#define BYTE_TO_MiB(bytes)  (bytes / 1048576)
#define BYTE_TO_KiB(bytes)  (bytes / 1024)

typedef struct SMAP_entry_st
{
    /*
        Base address 
        The begining of the memory map
    */
    uint64_t BaseAddress;

    /*
        Length in Byte
    */
    uint64_t Length;

    /*
    Types values :
        1 Usable memory
        2 Reserved memory
        3 ACPI reclaimed memory
        4 ACPI NVS Memory
        5 Aera contining bad memory
    */
    uint32_t Type;

    uint32_t ACPI;
} __attribute__((packed)) SMAP_entry;