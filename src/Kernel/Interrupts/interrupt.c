#include "interrupt.h"

extern void _set_idt();
extern void _load_idt(void* idt);
extern void* isr_table[];

struct IDT_Gates
{
    uint16_t offset_low;    // offset bits 0 .. 15
    uint16_t selector;      // a cpde segment in GDT or LDT
    uint8_t ist;            // interrupt stack table offset, bits 0 .. 2 rest of bits zero
    uint8_t flags;          // flags : type of attributes
    uint16_t offset_mid;    // offset 16 .. 31
    uint32_t offset_hight;  // offset 32 .. 63
    uint32_t zero;          // reserved
} __attribute__ ((packed));

struct IDT_Pointer
{
    uint16_t length;
    uint64_t address;
} __attribute__ ((packed));