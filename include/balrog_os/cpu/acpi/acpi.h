#pragma once
#include <stdint.h>
#include "acpispec/tables.h"

/*
Advanced Configuration and Power Interface

References :
ACPI : https://wiki.osdev.org/ACPI
RSDP : https://wiki.osdev.org/RSDP
RSDT : https://wiki.osdev.org/RSDT
XSDT : https://wiki.osdev.org/XSDT
FADT : https://wiki.osdev.org/FADT
LAI : https://wiki.osdev.org/LAI
the list of functions : lai/include/lai/host.h

*/

/*
    After reflexion on shutdown and searching more about it, I wanted to do it properly.
    The problem is that it requires an AML interpretor and when I checked how to write one,
    it was a very heavy task (estimation ~10k lines).

    Thus I decided to dig how to make a light weight one but it wouldn't change the total workload
    So, even if my goal was to make everything by hand, I decided to use LAI instead or doing it by hand
    https://github.com/managarm/lai

    The other option would have been to botch something out around those lines :
    https://forum.osdev.org/viewtopic.php?t=16990

    But it wouldn't be a proper shutdown.

    The following commented out code is the legacy of my acpi structures,
    I keep them as documentation in the kernel, but I'll use LAI's structure to avoid code deviations,
    and as LAI use them it would be cleaner to use LAI's implementation instead of mine.
*/

/**
 * @brief Initialize the ACPI (Advanced Configuration and Power Interface)
 *
 */
int init_acpi();

/**
 * @brief poweroff the computer
 */
void acpi_power_off();

/**
 * @brief reboot the computer.
 */
void acpi_reboot();

/*
if 0 as C do not accept embedded comments.
*/
#if 0


/**
 * @brief RSDP is the 32Bit version
 *        XSDP is 64Bit available in RSDP v2
 *        all field preceded by x_ v2 check require a check before use.
 */
typedef struct __rsdp_descriptor_t
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
    uint32_t x_length;
    /*
    64-bit physical address of the XSDT table (eXtended System Descriptor Table).
    If you detect ACPI Version 2.0 you should use this table instead of RSDT even on IA-32,
    casting the address to uint32_t.
    */
    uint64_t x_xsdt_address;
    /*
    This field is used to calculate the checksum of the entire table, including both checksum fields.
    */
    uint8_t x_extended_checksum;
    /*
    3 bytes to be ignored in reading and that must not be written.
    */
    uint8_t x_reserved[3];
} __attribute__((packed)) rsdp_descriptor_t;

/**
 * The header at the start of every ACPI table, the RSDP excepted.
 * length is the header plus the data behind it, and it is also what
 * we run the checksum over.
 */
typedef struct __acpisdt_header_t
{
    /*
    Four characters, not null terminated. "RSDT", "XSDT", "FACP" for the FADT,
    "DSDT", "APIC" for the MADT.
    */
    char signature[4];
    /*
    The size of the whole table, this header included. We deduce from it how
    many entries a RSDT or a XSDT holds.
    */
    uint32_t length;
    /*
    The revision of this table, which is not the revision of the ACPI itself.
    */
    uint8_t revision;
    /*
    Same rule as the RSDP. Every byte of the table over length, added as a
    uint8_t, must give 0. If it does not the table must be ignored.
    */
    uint8_t checksum;
    /*
    The specification says: "An OEM-supplied string that identifies the OEM".
    */
    char oemid[6];
    /*
    An OEM-supplied string that the OEM uses to identify this particular table.
    */
    char oem_table_id[8];
    /*
    An OEM-supplied revision number.
    */
    uint32_t oem_revision;
    /*
    Vendor ID of the utility that created the table.
    */
    uint32_t creator_id;
    /*
    Revision of that utility.
    */
    uint32_t creator_revision;
} __attribute__((packed)) acpisdt_header_t;

/**
 * How ACPI 2.0+ says where something is, when it can be a port, a
 * physical address or a place in the PCI configuration space.
 * The FADT uses it for the reset register and for every x_ field.
 */
typedef struct __generic_address_structure_t
{
    /*
    Where the address points : 0 = system memory, 1 = i/o port space,
    2 = pci configuration space. Anything else is out of scope here.

    Value	Address Space
    0	    System Memory
    1	    System I/O
    2	    PCI Configuration Space
    3	    Embedded Controller
    4	    System Management Bus
    5	    System CMOS
    6	    PCI Device BAR Target
    7	    Intelligent Platform Management Infrastructure
    8	    General Purpose I/O
    9	    Generic Serial Bus
    0x0A	Platform Communication Channel
    0x0B to 0x7F	Reserved
    0x80 to 0xFF	OEM Defined
    */
    uint8_t address_space;
    /*
    The size of the register, in bits.

    Value	Access size
    0	    Undefined (legacy reasons)
    1	    Byte access
    2	    16-bit (word) access
    3	    32-bit (dword) access
    4	    64-bit (qword) access
    */
    uint8_t bit_width;
    /*
    Where the register starts inside the addressed unit, in bits.
    */
    uint8_t bit_offset;
    /*
    The access size to use | 1 = byte, 2 = word, 3 = dword, 4 = qword.
    0 means undefined, and then we access the register at its bit_width.
    */
    uint8_t access_size;
    /*
    The address itself, read according to address_space.
    */
    uint64_t address;
} __attribute__((packed)) generic_address_structure_t;

/**
 * Fixed ACPI Description Table. We find it in the RSDT or the XSDT
 * under the signature "FACP", and not "FADT".
 *
 * It holds what the shutdown needs : the ports to write to, the way to
 * turn ACPI on, and where the DSDT is.
 *
 * All field preceded by x_ are the ACPI 2.0+ 64bit version of a 32bit
 * field above. We use them when they are not zero, and that is the test
 * to make : a firmware can say revision 2 and still leave one at zero.
 */
typedef struct __fadt_t
{
    acpisdt_header_t h;
    /*
    Physical address of the FACS, which the shutdown does not need.
    */
    uint32_t firmware_ctrl;
    /*
    Physical address of the DSDT. The \_S5_ object is in there, and so are the
    SLP_TYPa and SLP_TYPb values we write to power off.
    */
    uint32_t dsdt;
    /*
    Field used in ACPI 1.0, no longer in use, for compatibility only.
    */
    uint8_t reserved;

    /*
    This field contains a value which should address you to a power management profile.
    For example if it contains 2, the computer is a laptop
    and you should configure power management in power saving mode.

        Value	Meaning
        0	    Unspecified
        1	    Desktop
        2	    Mobile
        3	    Workstation
        4	    Enterprise Server
        5	    SOHO Server
        6	    Aplliance PC
        7	    Performance Server
        >7	    Reserved
     */
    uint8_t preferred_power_management_profile;
    /*
    The interrupt the system control interrupt is wired to.
    */
    uint16_t sci_interrupt;
    /*
    The port where we write acpi_enable when the machine boots in legacy mode.
    Zero means we have no way to switch it on. It is not an error, most
    machines are already in acpi mode.
    */
    uint32_t smi_command_port;
    /*
    The value we write to smi_command_port to ask for acpi mode. The firmware
    answers through an SMI and takes its time, so after that we have to poll
    SCI_EN in pm1a_control_block until it goes to 1.
    */
    uint8_t acpi_enable;
    /*
    The same, to go back to legacy mode.
    */
    uint8_t acpi_disable;

    uint8_t s4bios_req;
    uint8_t pstate_control;

    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    /*
    The power management control register. This is where we write
    SLP_TYPa | SLP_EN to power off, and where we read SCI_EN.
    */
    uint32_t pm1a_control_block;
    /*
    The second half of the same register, on the machines that split it. It is
    often zero, so we only write it when it is not. Writing it without the test
    writes to the port 0.
    */
    uint32_t pm1b_control_block;

    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    /*
    The width of pm1a_control_block, in bytes.
    */
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t c_state_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;
    /*
    Reserved in ACPI 1.0, used since ACPI 2.0+.
    */
    uint16_t boot_architecture_flags;

    uint8_t reserved2;
    /*
    Bit 10 tells us the reset register below can be used. The rest says what the
    machine supports, we don't need it to power off.
    */
    uint32_t flags;
    /*
    Where we write reset_value to restart the machine, if the bit 10 of flags
    is set and the table revision is 2 or more.
    */
    generic_address_structure_t reset_reg;

    uint8_t reset_value;
    uint8_t reserved3[3];

    uint64_t x_firmware_control;
    /*
    The 64bit dsdt. We use it instead of dsdt when it is not zero.
    */
    uint64_t x_dsdt;

    generic_address_structure_t x_pm1a_event_block;
    generic_address_structure_t x_pm1b_event_block;
    /*
    The 64bit pm1a_control_block. We use it when it is not zero. It is a generic
    address, so we read address_space first, it is not always a port.
    */
    generic_address_structure_t x_pm1a_control_block;
    generic_address_structure_t x_pm1b_control_block;
    generic_address_structure_t x_pm2_control_block;
    generic_address_structure_t x_pm_timer_block;
    generic_address_structure_t x_gpe0_block;
    generic_address_structure_t x_gpe1_block;
} __attribute__((packed)) fadt_t;

#endif
