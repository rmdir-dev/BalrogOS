#pragma once
#include <stdint.h>

/*
Advanced Configuration and Power Interface

References :
ACPI : https://wiki.osdev.org/ACPI
RSDP : https://wiki.osdev.org/RSDP
XSDT : https://wiki.osdev.org/XSDT

*/

/**
 * @brief 
 * 
 */
typedef struct __rsdp_escriptor
{
    /*
    This 8-byte string (not null terminated!) must contain "RSD PTR ". It stands on a 16-byte boundary.
    */
    char signature[8];
    /*
    The value to add to all the other bytes (of the Version 1.0 table) to calculate the Checksum of the table. 
    If this value added to all the others and casted to byte isn't equal to 0, the table must be ignored.
    */
    uint8_t checksum;
    /*
    The specification says: "An OEM-supplied string that identifies the OEM".
    */
    char oemid[6];
    /*
    The revision of the ACPI. Larger revision numbers are backward compatible to lower revision numbers. 
    The ACPI Version can be detected using the Revision field in the RSDP. If this field contains 0, then ACPI Version 1.0 is used. 
    For subsequent versions (ACPI version 2.0 to 6.1), the value 2 is used. The exact version of ACPI can be deduced via the FADT table.
    */
    uint8_t revision;
    /*
    32-bit physical address of the RSDT table.
    */
    uint32_t rsdtaddress;
    // VERSION 2 ONLY
    /*
    The size of the entire table since offset 0 to the end.
    */
    uint32_t length;
    /*
    64-bit physical address of the XSDT table (eXtended System Descriptor Table). 
    If you detect ACPI Version 2.0 you should use this table instead of RSDT even on IA-32, 
    casting the address to uint32_t.
    */
    uint64_t xsdt_address;
    /*
    This field is used to calculate the checksum of the entire table, including both checksum fields.
    */
    uint8_t extended_checksum;
    /*
    3 bytes to be ignored in reading and that must not be written.
    */
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_descriptor_t;

/**
 * @brief Initialize the ACPI (Advanced Configuration and Power Interface)
 * 
 */
void init_acpi();