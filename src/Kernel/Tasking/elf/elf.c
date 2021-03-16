#include "BalrogOS/Tasking/elf/elf.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "klib/IO/kprint.h"
#include <stddef.h>
#include <string.h>

static inline void _elf_load_prog(elf_program* prog, uint8_t* data, page_table* PML4T, uint32_t flags)
{
    uint64_t fsize = prog->p_memsz;
    uint8_t* phys = 0;
    uint8_t* vaddr = prog->p_vaddr;
    uint64_t offset = prog->p_offset;
    uint64_t alloc = 0;
    uint64_t shift = 0;
    uint16_t size = 0;

    while(fsize > 0)
    {
        shift = ((uintptr_t)vaddr) % 0x1000;
        size = fsize > 0x1000 ? (PAGE_SIZE - shift) : fsize;

        phys = vmm_get_page(PML4T, vaddr);
        
        if(phys == 0)
        {
            phys = pmm_calloc();
            vmm_set_page(PML4T, vaddr, phys, flags);
        }

        vaddr += size;
        fsize -= size;
    }
    
    fsize = prog->p_filesz;
    vaddr = prog->p_vaddr;

    while(fsize > 0)
    {
        shift = ((uintptr_t)vaddr) % 0x1000;
        size = fsize > 0x1000 ? (PAGE_SIZE - shift) : fsize;

        phys = vmm_get_page(PML4T, vaddr);

        if(phys == 0)
        {
            kprint("should not prompt 0%p\n", vaddr);
            phys = pmm_calloc();
            vmm_set_page(PML4T, vaddr, phys, flags);
        }

        memcpy(PHYSICAL_TO_VIRTUAL(phys) + shift, &data[offset], size);
        
        vaddr += size;
        fsize -= size;
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