#pragma once
#include "balrog_os/boot/uefi.h"
#include "balrog_os/boot/uefi_memory.h"
#include "balrog_os/boot/uefi_output.h"
#include "balrog_os/boot/uefi_boot_services.h"

/*
    LOADED IMAGE | tells us which device we were loaded from

OSDev, Loading files under UEFI § Volume Handle :
https://wiki.osdev.org/Loading_files_under_UEFI#Volume_Handle

"You probably want to use the same file system that your application was
loaded from, so you need to use the image handle provided to your efi_main.
Then you need to use the EFI_LOADED_IMAGE_PROTOCOL to figure out the device
your application resides on."
*/

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5b1b31a1, 0x9562, 0x11d2, { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

typedef struct
{
    uint32_t revision;
    EFI_HANDLE parent_handle;
    void* system_table;
    EFI_HANDLE device_handle;
    void* file_path;
    void* reserved;
    uint32_t load_options_size;
    void* load_options;
    void* image_base;
    uint64_t image_size;
    EFI_MEMORY_TYPE image_code_type;
    EFI_MEMORY_TYPE image_data_type;
    void* unload;
} EFI_LOADED_IMAGE_PROTOCOL;

/*
    CONFIGURATION TABLES | where the acpi rsdp is

OSDev, RSDP § Detecting the RSDP : https://wiki.osdev.org/RSDP#Detecting_the_RSDP

"If you're using UEFI, you can find it somewhere in EFI_SYSTEM_TABLE. Thus,
there's no need for searching the RAM.

Note: The standard methods to find the RSDP may not work on UEFI systems.
Because of that, finding it inside EFI_SYSTEM_TABLE is the correct and reliable
method (see ACPI 6.2 section 5.2.5.2 'Finding the RSDP on UEFI Enabled
Systems')."

OSDev, RSDP § Detecting ACPI Version : https://wiki.osdev.org/RSDP#Detecting_ACPI_Version

"The ACPI Version can be detected using the Revision field in the RSDP. If this
field contains 0, then ACPI Version 1.0 is used. For subsequent versions (ACPI
version 2.0 to 6.1), the value 2 is used [1]. The exact version of ACPI can be
deduced via the FADT table."

[1] http://www.uefi.org/sites/default/files/resources/ACPI_6_1.pdf

BalrogOS : the acpi 1.0 entry is there to be fallen back on and nothing else.
A firmware that still publishes only this one gives a 20 byte rsdp with
revision 0, so an rsdt and no xsdt, which the kernel handles anyway because
that is what seabios gives us on the bios side.
*/

#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

#define EFI_ACPI_10_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct
{
    EFI_GUID vendor_guid;
    void* vendor_table;
} EFI_CONFIGURATION_TABLE;

/*
    SYSTEM TABLE | what the firmware hands us

OSDev, UEFI § System discovery : https://wiki.osdev.org/UEFI#System_discovery

"When UEFI firmware calls a UEFI application's entry point function, it passes
a "System Table" structure, which contains pointers to all of the system's ACPI
tables, memory map, and other information relevant to an OS. Legacy tables
(like MP tables) may not be present in memory."
*/

typedef struct
{
    EFI_TABLE_HEADER hdr;
    CHAR16* firmware_vendor;
    uint32_t firmware_revision;
    EFI_HANDLE console_in_handle;
    void* con_in;
    EFI_HANDLE console_out_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con_out;
    EFI_HANDLE standard_error_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* std_err;
    void* runtime_services;
    EFI_BOOT_SERVICES* boot_services;
    uint64_t number_of_table_entries;
    EFI_CONFIGURATION_TABLE* configuration_table;
} EFI_SYSTEM_TABLE;
