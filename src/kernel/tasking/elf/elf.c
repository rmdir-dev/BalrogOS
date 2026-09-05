#include "balrog_os/tasking/elf/elf.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/debug/debug_output.h"
#include <stddef.h>
#include <string.h>

static inline void __elf_load_prog(elf_program* prog, uint8_t* data, page_table* PML4T, uint32_t flags)
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
            kernel_debug_output(KDB_LVL_CRITICAL, "elf : 0%p was not mapped by the memsz pass, should not prompt", vaddr);
            phys = pmm_calloc();
            vmm_set_page(PML4T, vaddr, phys, flags);
        }

        memcpy(P2V(phys) + shift, &data[offset], size);
        
        vaddr += size;
        fsize -= size;
        offset += size;
    }    
}

int elf_load_binary(elf_header* header, uint8_t* data, page_table* PML4T, uint32_t flags)
{
    if(header->ei_mag != ELF_MAGIC)
    {
        kernel_debug_output(KDB_LVL_ERROR, "elf : bad magic 0%x, expected 0%x", header->ei_mag, ELF_MAGIC);
        return -1;
    }

    if(header->ei_class != ELF_CLASS_64)
    {
        kernel_debug_output(KDB_LVL_ERROR, "elf : class %d is not 64 bit", header->ei_class);
        return -1;
    }

    if(header->e_machine != ELF_MACHINE_X86_64)
    {
        kernel_debug_output(KDB_LVL_ERROR, "elf : machine 0%x is not x86-64", header->e_machine);
        return -1;
    }

    kernel_debug_output(KDB_LVL_VERBOSE, "elf : entry 0%p, %d program headers at 0%x",
            header->e_entry, header->e_phnum, header->e_phoff);

    uint64_t ph_offset = header->e_phoff;
    elf_program* prog = &data[ph_offset];
    for(size_t i = 0; i < header->e_phnum; i++)
    {
        if(prog[i].p_type == ELF_PT_LOAD)
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "elf : load 0%p, filesz %d memsz %d, offset 0%x, flags 0%x",
                    prog[i].p_vaddr, prog[i].p_filesz, prog[i].p_memsz,
                    prog[i].p_offset, prog[i].p_flags);
            __elf_load_prog(&prog[i], data, PML4T, flags);
        }
    }
    return 0;
}