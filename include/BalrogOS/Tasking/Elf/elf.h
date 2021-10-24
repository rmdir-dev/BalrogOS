#pragma once

#include "BalrogOS/Memory/vmm.h"
#include <stdint.h>

#define ELF_MAGIC   0x464c457f

#define ELF_PT_NULL     0x00000000
#define ELF_PT_LOAD     0x00000001
#define ELF_PT_DYNAMIC  0x00000002
#define ELF_PT_INTERP   0x00000003
#define ELF_PT_NOTE     0x00000004
#define ELF_PT_SHLIB    0x00000005
#define ELF_PT_PHDR     0x00000006
#define ELF_PT_TLS      0x00000007
#define ELF_PT_LOOS     0x60000000
#define ELF_PT_HIOS     0x6FFFFFFF
#define ELF_PT_LOPROC   0x70000000
#define ELF_PT_HIPROC   0x7FFFFFFF

typedef struct _elf_header
{
    uint32_t ei_mag;        // Elf Magic number
    uint8_t ei_class;       // This byte is set to either 1 or 2 to signify 32- or 64-bit format, respectively.
    uint8_t ei_data;        // This byte is set to either 1 or 2 to signify little or big endianness, respectively. 
                            // This affects interpretation of multi-byte fields starting with offset 0x10.
    uint8_t ei_version;     // Set to 1 for the original and current version of ELF.
    uint8_t ei_osabi;       // Identifies the target operating system ABI (application binary interface). (read elf.h for more informations)
    uint8_t ei_abiversion;  // 
    uint8_t ei_pad[7];      // Padding
    uint16_t e_type;        // 
    uint16_t e_machine;     // machine type (look elf.h for more informations)
    uint32_t e_version;     // Set to 1 for the original version of ELF.
    uint64_t e_entry;       // entry point virtual address
    uint64_t e_phoff;       // program header offset
    uint64_t e_shoff;       // section header offset
    uint32_t e_flags;       // nterpretation of this field depends on the target architecture.
    uint16_t e_ehsize;      // Contains the size of this header, normally 64 Bytes for 64-bit and 52 Bytes for 32-bit format.
    uint16_t e_phentsize;   // program entry size
    uint16_t e_phnum;       // number of program
    uint16_t e_shentsize;   // section entry size
    uint16_t e_shnum;       // number of sections
    uint16_t e_shstrndx;    // Contains index of the section header table entry that contains the section names.
} __attribute__((packed)) elf_header;

typedef struct _elf_program
{
    uint32_t p_type;        // program type
    uint32_t p_flags;       // 
    uint64_t p_offset;      // program offset
    uint64_t p_vaddr;       // virtual address
    uint64_t p_paddr;       // physical address (needed by some system)
    uint64_t p_filesz;      // program size
    uint64_t p_memsz;       // size in memory
    uint64_t p_align;       // 0 and 1 specify no alignment. 
                            // Otherwise should be a positive, integral power of 2, 
                            // with p_vaddr equating p_offset modulus p_align.
} __attribute__((packed)) elf_program;

typedef struct _elf_section
{
    uint32_t sh_name;       // An offset to a string in the .shstrtab section
                            // that represents the name of this section.
    uint32_t sh_type;       // Identifies the type of this header.
    uint64_t sh_flags;      // Identifies the attributes of the section.
    uint64_t sh_addr;       // Virtual address of the section in memory, for sections that are loaded.
    uint64_t sh_offset;     // section offset
    uint64_t sh_size;       // section size
    uint32_t sh_link;       // 
    uint32_t sh_info;       // 
    uint64_t sh_addralign;  // Contains the required alignment of the section.
                            // This field must be a power of two.
    uint64_t sh_entsize;    // entry size
} __attribute__((packed)) elf_section;

int elf_load_binary(elf_header* header, uint8_t* data, page_table* PML4T, uint32_t flags);