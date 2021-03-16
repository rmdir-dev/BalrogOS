#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/CPU/TSS/tss.h"
#include "BalrogOS/Memory/memory.h"
#include "klib/IO/kprint.h"

gdt_entry gdt[7];
gdt_ptr gdtp;

tss_entry tss;

extern void flush_gdt(gdt_ptr* gdtp);
extern void flush_tss();

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned long base, unsigned long limit,
                           unsigned char access, unsigned char gran)
{
    /* Setup the descriptor base address */
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void install_tss(gdt_entry* gdt)
{
    /* get the tss base    */
    uint64_t base = (uint64_t) &tss;
    /* get the tss limit */
    uint32_t limit = sizeof(tss_entry);

    /* setup limit */
    gdt->limit_low = limit & 0xffff;
    /* setup base */
    gdt->base_low = base & 0xffff;
    gdt->base_middle = (base >> 16) & 0xff;
    gdt->base_high = (base >> 24) & 0xff;
    
    gdt->access = 0b11101001;
    gdt->granularity = 0b0;

    /*
    Add tss last part of the 64bit address
    */
    gdt++;
    uintptr_t* last_index = gdt;
    *last_index = (base >> 32) & 0xffffffff;
}

void init_gdt()
{
    gdtp.limit = (sizeof(gdt_entry) * 7) - 1;
    gdtp.base = (uint64_t) &gdt[0];

    tss.io_mba = sizeof(tss_entry);
    asm volatile("mov %%rsp, %%rax": "=a"(tss.rsp0));

    /* NULL */
    gdt_set_gate(0, 0, 0, 0, 0);
    /* KERNEL CODE */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);
    /* KERNEL DATA */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xAF);
    /* USER CODE */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF);
    /* USER DATA */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xAF);
    /* install tss on gdt[5] and gdt[6] */
    install_tss(&gdt[5]);

    /* load and flush new gdt and tss to the CPU */
    flush_gdt(&gdtp);
    flush_tss();
}