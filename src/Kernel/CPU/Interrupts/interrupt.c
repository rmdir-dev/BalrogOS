#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Ports/ports.h"
#include <string.h>
#include "klib/IO/kprint.h"
#include "BalrogOS/Debug/debug_output.h"

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

static struct IDT_Gates gates[TOTAL_NBR_INTERRUPT];
static struct IDT_Pointer idt_ptr = { 0, 0 };
static interrupt_handler int_handlers[TOTAL_NBR_INTERRUPT];

static void set_idt_entry(uint32_t id, void* vector, uint16_t selector, uint8_t ist, uint8_t flags)
{
    uintptr_t v = (uintptr_t)vector;
    gates[id].offset_low = v & 0xffff;
    gates[id].selector = selector;
    gates[id].ist = ist;
    gates[id].flags = flags;
    gates[id].offset_mid = (v >> 16) & 0xffff;
    gates[id].offset_hight = (v >> 32) & 0xffffffff;
}

void init_interrupt()
{
    if(!idt_ptr.address)
    {
        /*
        Remap the PIC (Programmable Interrupt Controller)
        Remap IRQs 0-7    to    ISRs 32-39
        and   IRQs 8-15   to    ISRs 40-47
        */
        out_byte(0x20, 0x11);
        out_byte(0xA0, 0x11);
        out_byte(0x21, 0x20);
        out_byte(0xA1, 0x28);
        out_byte(0x21, 0x04);
        out_byte(0xA1, 0x02);
        out_byte(0x21, 0x01);
        out_byte(0xA1, 0x01);
        out_byte(0x21, 0xff);
        out_byte(0xA1, 0xff);
        
        /*
        Clear gates and handler array
        Make sure that we don't have any byte not to 0
        */
        memset(gates, 0, sizeof(gates));
        memset(int_handlers, 0, sizeof(int_handlers));

        for(uint32_t i = 0; i < TOTAL_NBR_INTERRUPT; i++)
        {
            set_idt_entry(i,        // ISR ID
                isr_table[i],       // poiner to the corresponding ISR
                0x8,                // kernel segment code offset sector
                0,                  //
                (IDT_PRESENT |      // INTERRUPT IS PRESENT = can be use
                IDT_DPL_0 |         // INTERRUPT privilege level 0 = highest privilege to call this interrupt
                IDT_INTERRUPT));    // INTERRUPT gate https://wiki.osdev.org/Interrupt_Descriptor_Table#I386_Interrupt_Gate
        }

        idt_ptr.address = (uint64_t)gates;
        idt_ptr.length = sizeof(gates) - 1;
    }

    _load_idt(&idt_ptr);
}

interrupt_handler register_interrupt_handler(uint32_t id, interrupt_handler handler)
{
    interrupt_handler old = int_handlers[id];
    int_handlers[id] = handler;
    return old;
}

void set_interrupt_flag(uint32_t id, uint8_t flags)
{
    if(id >= INT_SYSCALL && id < INT_APIC_SPUR)
    {
        gates[id].flags = flags;
    }

    //_load_idt(&idt_ptr);
}

interrupt_regs* kernel_interrupt_handler(interrupt_regs* stack_frame)
{
    /*
        Check if there is an handler installed on the int_handler array
    */
    if(!int_handlers[stack_frame->interrupt_no])
    {
        // if not print a message and loop
        kernel_debug_output(KDB_LVL_CRITICAL, "interrupt %d has no handler!\n", stack_frame->interrupt_no);
        kernel_debug_output(KDB_LVL_CRITICAL, "RFLAGS 0%x \n", stack_frame->rflags);

        while(1){}
    }
    /* execute ISR */
    int_handlers[stack_frame->interrupt_no](stack_frame);
    
    return stack_frame;
}