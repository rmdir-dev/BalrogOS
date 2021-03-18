#pragma once

/*
ADDRESSES
*/
#define KERNEL_OFFSET 0xFFFFFF8000000000

/*
Balrog Memory Map :
    Start			    End			        Size		Use
    -----------------------------------------------------------------------
    0000000000000000	ffffff7fffffffff	 255.5TB	user
    ffffff8000000000	ffffff8000017fff	 95KB		kernel stack, code
    ffffff8000018000	ffffff800009fbff	 542KB		kernel logical heap
    ffffff800009fc00	ffffff80000fffff	 385KB		RESERVED
    ffffff8000100000	ffffff9fffffffff	 128GB		MEMORY (real size : 128GiB - 1MiB)
    ffffffa000000000    ffffffbfffffffff     128GB      Kernel open files cache
    ffffffc000000000	ffffffdfffffffff	 128GB		Kernel virtual heap
    ffffffe000000000	ffffffffffffffff	 128GB		Process kernel stack space

Balrog Process memory map :
    Start			    End			        Size		Use
    -----------------------------------------------------------------------
    0000000000400000	000055c0603d2fff	 85TB	    code
    000055c0603d3000	00007ffd0d812000	 42TB		heap
    2MB buffer
    00007ffd0da12000	00007ffd0e212000	 8MB		stack
    00007ffd0da13000	00007ffd0e213fff	 4KB		proc meta data (argvs)

Max Memory size :
    128GiB
*/

#include <stdint.h>

/*
addr = 0x101000
0x101000 & ~0xFFFFFF8000000000
0x101000 & 7FFFFFFFFF
= 0x101000
*/
#define VIRTUAL_TO_PHYSICAL(addr) ((uintptr_t)addr & ~KERNEL_OFFSET)

/*
addr = 0x101000
0x101000 | 0xFFFFFF8000000000
= 0xFFFFFF8000101000
*/
#define PHYSICAL_TO_VIRTUAL(addr) ((uintptr_t)addr | KERNEL_OFFSET)

/*
----------------------------------------------------------------------------------
            PAGING
----------------------------------------------------------------------------------
*/
/*
    0xFfedcba987654321

    v                     V PML4 IDX V PDPT IDX v PDT IDX  v PT INDEX  vOFFSET
    0b1111 1111 1110 1101 1100 1011 1010 1001 1000 0111 0110 0101 0100 0011 0010 0001

    13 to 21 lower bits = the Page Table index 0x654 & 0x1ff = 0x154
                        each page table contain 512 entries.
                        0x1ff = 512
    12 lower bits = the offset 0x321 (1 page is 4KiB so 0x1000 Bytes)
                    so the offset can go from 0x000 to 0xfff which is the
                    bottom and top of the page.
*/
#define PML4T_OFFSET(addr)  ((((uintptr_t)(addr)) >> 39) & 0x1ff)
#define PDPT_OFFSET(addr)   ((((uintptr_t)(addr)) >> 30) & 0x1ff)
#define PDT_OFFSET(addr)    ((((uintptr_t)(addr)) >> 21) & 0x1ff)
#define PT_OFFSET(addr)     ((((uintptr_t)(addr)) >> 12) & 0x1ff)

#define PML4T_TO_VIRT(addr) (((uintptr_t)(addr)) << 39)
#define PDPT_TO_VIRT(addr)  (((uintptr_t)(addr)) << 30)
#define PDT_TO_VIRT(addr)   (((uintptr_t)(addr)) << 21)
#define PT_TO_VIRT(addr)    (((uintptr_t)(addr)) << 12)

#define STRIP_FLAGS(addr)       (((uintptr_t)addr) & ~PAGE_FLAG_MASK)
#define ADD_FLAGS(addr, flags)   (((uintptr_t)addr) | (flags & PAGE_FLAG_MASK))

#define PAGE_FLAG_MASK      0xfff

#define   PAGE_PRESENT        0x001
#define   PAGE_WRITE          0x002
#define   PAGE_USER           0x004
#define   PAGE_WRITETHROUGH   0x008
#define   PAGE_NOCACHE        0x010
#define   PAGE_ACCESSED       0x020
#define   PAGE_DIRTY          0x040
#define   PAGE_HUGE           0x080
#define   PAGE_GLOBAL         0x100 

/*
----------------------------------------------------------------------------------
            CONVERSION
----------------------------------------------------------------------------------
*/

#define ONE_GiB 1073741824
#define ONE_MiB 1048576
#define ONE_kiB 1024

#define BYTE_TO_GiB(bytes)  (bytes / ONE_GiB)
#define BYTE_TO_MiB(bytes)  (bytes / ONE_MiB)
#define BYTE_TO_KiB(bytes)  (bytes / ONE_kiB)

#define USABLE_MEMORY               1
#define RESERVED_MEMORY             2
#define ACPI_RECLAIMEBLE_MEMORY     3
#define ACPI_NVS_MEMORY             4
#define AREA_BAD_MEMORY             5

/*
1 page = 4096 bytes
*/
#define PAGE_SIZE 4096

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