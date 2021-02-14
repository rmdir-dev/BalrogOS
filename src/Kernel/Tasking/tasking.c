#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/CPU/RFLAGS/rflag.h"
#include <string.h>

#define PROCESS_STACK_TOP   0x00007ffd0e212000
#define PROCESS_HEAP_BOTTOM 0x000055c0603d3000
#define PROCESS_TEXT        0x0000000000400000

uint64_t next_pid = 0;

typedef struct task_register_t
{
    //////////////////////////////////////////////////////////
    //                  STACK TOP
    //////////////////////////////////////////////////////////
    uint64_t rbp;           //
    uint64_t r15;           //
    uint64_t r14;           //
    uint64_t r13;           //
    uint64_t r12;           //
    uint64_t r11;           //
    uint64_t r10;           //
    uint64_t r9;            //
    uint64_t r8;            //
    uint64_t rdi;           //
    uint64_t rsi;           //
    uint64_t rdx;           //
    uint64_t rcx;           //
    uint64_t rbx;           //
    uint64_t rax;           // ALl the registers to handle ISR

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
} __attribute__((packed)) task_register;

/**
 * @brief Create a process object 
 * 
 * @param name 
 * @param addr 
 * @return process* 
 */
process* create_process(char* name, uintptr_t addr, uint8_t mode)
{
    process* proc = vmalloc(sizeof(process));
    memset(proc, 0, sizeof(process));
    proc->name = name;
    proc->pid = ++next_pid;
    proc->rip = addr;
    proc->state = PROCESS_STATE_ALIVE;
    proc->PML4T = pmm_calloc();
    proc->exec = 0;
    uintptr_t* virt = PHYSICAL_TO_VIRTUAL(proc->PML4T); // Kernel space
    virt[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE; // to change process won't be able to write into kernel space
    /*
    TEXT
    */
    uintptr_t text = pmm_calloc();
    uintptr_t phys = VIRTUAL_TO_PHYSICAL(addr);
    vmm_set_page(proc->PML4T, PROCESS_TEXT, text, PAGE_USER | PAGE_PRESENT);
    memcpy(PHYSICAL_TO_VIRTUAL(text), addr, 4096);

    /*
    HEAP
    */
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_HEAP_BOTTOM, phys, PAGE_USER | PAGE_PRESENT | PAGE_WRITE);

    /*
    STACK
    */
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x1000, phys, PAGE_USER | PAGE_PRESENT | PAGE_WRITE);

    /*
        SETUP THE STACK
    */
    proc->stack_top = PROCESS_STACK_TOP - 1;
    proc->rsp = PROCESS_STACK_TOP - sizeof(task_register) - 1;
    virt = PHYSICAL_TO_VIRTUAL(((uint8_t*)phys) + 4095 - sizeof(task_register));
    
    task_register* stack = virt;

    if(mode == 3)
    {
        proc->rip = PROCESS_TEXT;
        stack->ss = SEG_UDATA | 3;
        stack->rsp = proc->stack_top;
        stack->rflags = RFLAG_IF;
        stack->cs = SEG_UCODE | 3;
    } else
    {
        stack->ss = SEG_KDATA;
        stack->rsp = proc->stack_top;
        stack->rflags = RFLAG_IF;
        stack->cs = SEG_KCODE;
    }

    stack->rip = proc->rip;
    stack->r15 = 0;
    stack->r14 = 0;
    stack->r13 = 0;
    stack->r12 = 0;
    stack->r11 = 0;
    stack->r10 = 0;
    stack->r9 = 0;
    stack->r8 = 0;
    stack->rbp = proc->stack_top;
    stack->rdi = 0;
    stack->rsi = 0;
    stack->rdx = 0;
    stack->rcx = 0;
    stack->rbx = 0;
    stack->rax = 0;

    return proc;
}