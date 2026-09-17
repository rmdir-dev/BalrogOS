#pragma once
#include "balrog_os/boot/uefi.h"

/*
    MEMORY

OSDev, UEFI § Memory : https://wiki.osdev.org/UEFI#Memory

"The memory map returned by UEFI will mark the memory areas which drivers use.
Once your OS loader finished, your kernel is allowed to reuse the memory where
the boot loader was loaded.
The memory types are Efi{Loader/BootServices/RuntimeServices}{Code/Data}.
After exiting the boot services, you may reuse whatever non-read-only memory
the boot drivers used.
However, memory used by the runtime drivers must never be touched - the runtime
drivers stay active and loaded for as long as the computer runs."
*/

typedef enum
{
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum
{
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress
} EFI_ALLOCATE_TYPE;

/*
    MEMORY MAP

OSDev, Detecting Memory (x86) § What about on UEFI? :
https://wiki.osdev.org/Detecting_Memory_(x86)#What_about_on_UEFI?

"On UEFI, you have 'BootServices->GetMemoryMap'. This function is similar to
E820 and is the only solution on new UEFI machines. Basically, to use, first
you call it once to get the size of the memory map. Then you allocate a buffer
of that size, and then call again to get the map itself. Watch out, by
allocating memory you could increase the size of the memory map. Considering
that a new allocation can split a free memory area into two, you should add
space for 2 additional memory descriptors. It returns an array of
EFI_MEMORY_DESCRIPTORs. [...]
To traverse them, you can use the NEXT_MEMORY_DESCRIPTOR macro."
*/
typedef struct
{
    uint32_t type;
    uint32_t pad;
    EFI_PHYSICAL_ADDRESS physical_start;
    EFI_VIRTUAL_ADDRESS virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} EFI_MEMORY_DESCRIPTOR;
