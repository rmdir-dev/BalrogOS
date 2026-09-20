#pragma once

/*
ADDRESSES
*/
#define KERNEL_MAP_BASE     0xFFFFFF8000000000
#define KERNEL_TEXT_BASE    0xFFFFFFFF80000000

#define KERNEL_OFFSET       KERNEL_MAP_BASE

/*
Balrog Memory Map :
    Start                           End                         Size            Use
    -----------------------------------------------------------------------
    0000000000000000    ffffff7fffffffff         255.5TiB       user
    ffffff8000000000    ffffff9fffffffff         128GiB         MEMORY, the linear window
    ffffffa000000000    ffffffbfffffffff         128GiB         Kernel open files cache
    ffffffc000000000    ffffffdfffffffff         128GiB         Kernel virtual heap
    ffffffe000000000    ffffffff7fffffff         126GiB         Process kernel stack space
    ffffffff80000000    ffffffffffffffff         2GiB           kernel text, rodata, data, bss and the NOLOAD areas

Balrog Process memory map :
    Start                           End                         Size            Use
    -----------------------------------------------------------------------
    0000000000400000    000055c0603d2fff         85TiB      code
    000055c0603d3000    00007ffd0d812000         42TiB      heap
    2MB buffer
    00007ffd0da12000    00007ffd0e212000         8MiB       stack
    00007ffd0da13000    00007ffd0e213fff         4KiB       proc meta data (argvs)

Max Memory size :
    128GiB
*/

#include <stdint.h>

/*
the text window                                 the linear one
addr = 0xffffffff80101000                       addr = 0xffffff8000101000
0xffffffff80101000 - 0xffffffff80000000         0x101000 & ~0xFFFFFF8000000000
= 0x101000                                      = 0x101000
*/
static inline __attribute__((always_inline)) uintptr_t V2P(uintptr_t addr)
{
    return addr >= KERNEL_TEXT_BASE ? addr - KERNEL_TEXT_BASE : addr & ~KERNEL_MAP_BASE;
}

/*
Physical always goes back to the linear window and not the .text one
So P2V(V2P(x)) is not equal to x but both are pointing to the same physical
page as the first 8MiB are mapped in both windows, so both are valid and the same.
(linux does the same thing).

addr = 0x101000
0x101000 | 0xFFFFFF8000000000
= 0xFFFFFF8000101000
*/
static inline __attribute__((always_inline)) uintptr_t P2V(uintptr_t addr)
{
    return addr | KERNEL_MAP_BASE;
}

/*
P2V is an or, not an addition, so it only holds while the address fits in the
MEMORY window of the map above.

addr = 0x2000000000
0x2000000000 | 0xFFFFFF8000000000
= 0xFFFFFFA000000000
the open files cache, not memory

A bar or a framebuffer up there needs a window of its own.
*/
#define P2V_MAX 0x2000000000ULL

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

#define STRIP_FLAGS(addr)        (((uintptr_t)(addr)) & ~PAGE_FLAG_MASK)
#define GET_FLAGS(addr)          (((uintptr_t)(addr)) & PAGE_FLAG_MASK)
#define ADD_FLAGS(addr, flags)   (((uintptr_t)(addr)) | ((flags) & PAGE_FLAG_MASK))

#define PAGE_FLAG_MASK      0xfff

#define   PAGE_PRESENT          0x001
#define   PAGE_WRITE            0x002
#define   PAGE_USER             0x004
#define   PAGE_WRITETHROUGH     0x008
#define   PAGE_NOCACHE          0x010
#define   PAGE_ACCESSED         0x020
#define   PAGE_DIRTY            0x040
#define   PAGE_HUGE             0x080
// PAGE_GLOBAL is always ignored if the page is a PML4T
// It is also ignored on PDPT and PDT except if the PAGE_HUGE flag is set
#define   PAGE_GLOBAL           0x100


/*
    CUSTOM

    Balrog OS flags (set after PAGE_GLOBAL as these are available flag bits 9 to 11).
*/

/*
 *  Will be used for :
 *      - user accessible kernel pages :
 *          - framebuffer accessible from user space (not global but shared)
 *          - vDSO (Virtual Dynamic Shared Object) kernel's code accessible in ring 3 (e.g. clock get_current_time
 *              without interrupt).
 *      - shared libraries : libraries are not loaded at the same address by every process thus marking them
 *                           global would not clean the TLB of these translation and cause a process to crash
 *                           or have unexpected behaviour if the content of the shared library's page should
 *                           be something else.
 */
#define   PAGE_SHARED           0x200

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

/*
addr = 0x101abc
(0x101abc + 0xfff) & ~0xfff
= 0x102000
*/
#define PAGE_ALIGN_UP(addr) ((((uintptr_t)(addr)) + PAGE_SIZE - 1) & ~((uintptr_t)PAGE_SIZE - 1))

/*
addr = 0x101abc
0x101abc & ~0xfff
= 0x101000
*/
#define PAGE_ALIGN_DOWN(addr) (((uintptr_t)(addr)) & ~((uintptr_t)PAGE_SIZE - 1))

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