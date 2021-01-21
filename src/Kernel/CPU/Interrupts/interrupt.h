#pragma once

#include <stdint.h>

#define IDT_CALL        0x0c
#define IDT_INTERRUPT   0x0e
#define IDT_TRAP        0x0f
#define IDT_DPL_0       0x00
#define IDT_DPL_3       0x60
#define IDT_PRESENT     0x80

#define TOTAL_NBR_INTERRUPT 256

#define INT_PAGE_FAULT  0x0e

#define INT_IRQ_0       0x20
#define INT_IRQ_1       0x21
#define INT_IRQ_2       0x22
#define INT_IRQ_3       0x23
#define INT_IRQ_4       0x24
#define INT_IRQ_5       0x25
#define INT_IRQ_6       0x26
#define INT_IRQ_7       0x27
#define INT_IRQ_8       0x28
#define INT_IRQ_9       0x29
#define INT_IRQ_10      0x2a
#define INT_IRQ_11      0x2b
#define INT_IRQ_12      0x2c
#define INT_IRQ_13      0x2d
#define INT_IRQ_14      0x2e
#define INT_IRQ_15      0x2f
#define INT_IRQ_16      0x30
#define INT_IRQ_17      0x31
#define INT_IRQ_18      0x32
#define INT_IRQ_19      0x33
#define INT_IRQ_20      0x34
#define INT_IRQ_21      0x35
#define INT_IRQ_22      0x36
#define INT_IRQ_23      0x37

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

#define disable_interrupt() asm("cli")

#define enable_interrupt() asm("sti")