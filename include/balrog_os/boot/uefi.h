#pragma once
#include <stdint.h>

/*
UEFI bootloader

References :
UEFI       : https://wiki.osdev.org/UEFI
bare bones : https://wiki.osdev.org/UEFI_Bare_Bones
the spec   : https://uefi.org/specifications
*/

/*
OSDev, UEFI § Calling Conventions : https://wiki.osdev.org/UEFI#Calling_Conventions

"UEFI specifies the following calling conventions:
 - cdecl for x86 UEFI functions
 - Microsoft's 64-bit calling convention for x86-64 UEFI functions
 - SMC for ARM UEFI functions
This has two impacts on UEFI application developers:
 - A UEFI application's main entry point must expect to be called with the
   corresponding calling convention.
 - Any UEFI-provided functions that a UEFI application calls must be called
   with the corresponding calling convention.
Note that functions strictly internal to the application can use whatever
calling convention the developer chooses."
*/
#define EFIAPI __attribute__((ms_abi))

typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef void* EFI_EVENT;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;
typedef uint16_t CHAR16;
typedef uint8_t BOOLEAN;

/*
The high bit marks an error, so a success is a small number and an error is a
huge one. That is why we test against EFI_SUCCESS and not against a sign.
*/
#define EFI_SUCCESS             0
#define EFI_LOAD_ERROR          0x8000000000000001
#define EFI_INVALID_PARAMETER   0x8000000000000002
#define EFI_UNSUPPORTED         0x8000000000000003
#define EFI_BUFFER_TOO_SMALL    0x8000000000000005
#define EFI_NOT_FOUND           0x800000000000000E

#define EFI_ERROR(status) (((EFI_STATUS)(status)) >> 63)

typedef struct
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} EFI_GUID;

/*
TABLE HEADER, at the top of every table
*/

typedef struct
{
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} EFI_TABLE_HEADER;
