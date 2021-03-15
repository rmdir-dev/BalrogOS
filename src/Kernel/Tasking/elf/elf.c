#include "BalrogOS/Tasking/elf/elf.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "lib/IO/kprint.h"
#include <stddef.h>
#include <string.h>

static inline void _elf_load_prog(elf_program* prog, uint8_t* data, page_table* PML4T, uint32_t flags)
{
    uint64_t copied = 0;
    uint8_t* phys = 0;
    uint8_t* vaddr = prog->p_vaddr;
    uint64_t offset = prog->p_offset;
    uint64_t shift = 0;
    uint16_t size = 0;
    
    while(copied < prog->p_filesz)
    {
        shift = ((uintptr_t)vaddr) % 0x1000;
        size = PAGE_SIZE - shift;

        phys = vmm_get_page(PML4T, vaddr);
        
        if(phys == 0)
        {
            phys = pmm_calloc();
            vmm_set_page(PML4T, vaddr, phys, flags);
        }

        memcpy(PHYSICAL_TO_VIRTUAL(phys) + shift, &data[offset], size);
        
        vaddr += size;
        copied += size;
        offset += size;
    }
}

int elf_load_binary(elf_header* header, uint8_t* data, page_table* PML4T, uint32_t flags)
{
    
    uint64_t ph_offset = header->e_phoff;
    elf_program* prog = &data[ph_offset];
    for(size_t i = 0; i < header->e_phnum; i++)
    {
        if(prog[i].p_type == ELF_PT_LOAD)
        {
            _elf_load_prog(&prog[i], data, PML4T, flags);
        }
    }
    return 0;
}