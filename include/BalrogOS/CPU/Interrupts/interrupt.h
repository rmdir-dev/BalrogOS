#pragma once

#include <stdint.h>

#define IDT_CALL        0x0c // 0b0000 1100
#define IDT_INTERRUPT   0x0e // 0b0000 1110
#define IDT_TRAP        0x0f // 0b0000 1111
#define IDT_DPL_0       0x00 // 0b0000 0000
#define IDT_DPL_3       0x60 // 0b0110 0000
#define IDT_PRESENT     0x80 // 0b1000 0000

#define TOTAL_NBR_INTERRUPT 256

#define   INT_DE    0x00 // Divide by zero exception
#define   INT_DB    0x01 // Debug exception
#define   INT_NMI   0x02 // Non-maskable Interrupt exception
#define   INT_BP    0x03 // Beakpoint exception
#define   INT_OF    0x04 // Overflow exception
#define   INT_BR    0x05 // Bound Range exception
#define   INT_UD    0x06 // Invalid Opcode exception
#define   INT_NM    0x07 // Device not Available exception
#define   INT_DF    0x08 // Double Fault exception
#define   INT_CSO   0x09 // Coprocessor Segment Overrun exception
#define   INT_TS    0x0A // Invalid TSS exception
#define   INT_NP    0x0B // Segment not Present excepton
#define   INT_SS    0x0C // Stack exception
#define   INT_GP    0x0D // General Protection Fault exception
#define   INT_PF    0x0E // Page Fault exception
#define   INT_MF    0x10 // Floating Point exception pending
#define   INT_AC    0x11 // Alignment Check exception
#define   INT_MC    0x12 // Machine Check exception
#define   INT_XF    0x13 // SIMD Floating Point exception
#define   INT_SX    0x1E // Security exception

/* MASTER PIC */
#define INT_IRQ_0       0x20    // Programmable Interrupt Timer Interrupt
#define INT_IRQ_1       0x21    // Keyboard interrupt
#define INT_IRQ_2       0x22    // Cascade (used internally by the two PICs. never raised)
#define INT_IRQ_3       0x23    // COM2
#define INT_IRQ_4       0x24    // COM1
#define INT_IRQ_5       0x25    // LPT2
#define INT_IRQ_6       0x26    // Floppy Disk
#define INT_IRQ_7       0x27    // LPT1
/* SLAVE PIC */
#define INT_IRQ_8       0x28    // CMOS real-time clock
#define INT_IRQ_9       0x29    // Free for peripherals / legacy SCSI / NIC
#define INT_IRQ_10      0x2a    // Free for peripherals / SCSI / NIC
#define INT_IRQ_11      0x2b    // Free for peripherals / SCSI / NIC
#define INT_IRQ_12      0x2c    // PS2 Mouse
#define INT_IRQ_13      0x2d    // FPU / Coprocessor / Inter-processor
#define INT_IRQ_14      0x2e    // Primary ATA Hard Disk
#define INT_IRQ_15      0x2f    // Secondary ATA Hard Disk

#define INT_IRQ_16      0x30    // 
#define INT_IRQ_17      0x31    // 
#define INT_IRQ_18      0x32    // 
#define INT_IRQ_19      0x33    // 
#define INT_IRQ_20      0x34    // 
#define INT_IRQ_21      0x35    // 
#define INT_IRQ_22      0x36    // 
#define INT_IRQ_23      0x37    // 

#define   INT_APIC_TIMER     0x40
#define   INT_APIC_THERMAL   0x41
#define   INT_APIC_PERF      0x42
#define   INT_APIC_LINT0     0x43
#define   INT_APIC_LINT1     0x44
#define   INT_APIC_ERROR     0x45
#define   INT_APIC_SPUR      0xFF

#define INT_SYSCALL 0x80

typedef struct interrupt_registers
{
    //////////////////////////////////////////////////////////
    //                  STACK TOP
    //////////////////////////////////////////////////////////
    uint64_t rax;           // ALl the registers to handle ISR
    uint64_t rbx;           //
    uint64_t rcx;           //
    uint64_t rdx;           //
    uint64_t rsi;           //
    uint64_t rdi;           //
    uint64_t rbp;           //
    uint64_t r8;            //
    uint64_t r9;            //
    uint64_t r10;           //
    uint64_t r11;           //
    uint64_t r12;           //
    uint64_t r13;           //
    uint64_t r14;           //
    uint64_t r15;           //

    uint64_t interrupt_no;  // interrupt number
    uint64_t error_code;    //error code
    //////////////////////////////////////////////////////////
    //                  STACK BOTTOM
    //////////////////////////////////////////////////////////
    uint64_t rip;       // rip = next instruction
    uint64_t cs;        // code segment
    uint64_t rflags;    // rflags
    uint64_t rsp;       // stack pointer
    uint64_t ss;        // Stack segment
    //////////////////////////////////////////////////////////
    //                  STACK FRANE END
    //////////////////////////////////////////////////////////
} interrupt_regs;

typedef interrupt_regs *(*interrupt_handler)(interrupt_regs*);

/**
* @brief Initialize all 256 Interrupt Service Routine
*/
void init_interrupt();

/**
* @brief add a new ISR handler.
* @param id interrupt id 
* @param handler function pointer to ISR handler handler.
* @return return the previous ISR handler
*/
interrupt_handler register_interrupt_handler(uint32_t id, interrupt_handler handler);

/**
 * @brief Set new interrupt flag into the IDT and reload it.
 * 
 * @param id interrupt ID
 * @param flag intterrupt FLAGS
 */
void set_interrupt_flag(uint32_t id, uint8_t flag);

#define disable_interrupt() asm volatile("cli")

#define enable_interrupt() asm volatile("sti")