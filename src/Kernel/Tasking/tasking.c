#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
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
} task_register;

process* create_process(char* name, uintptr_t addr)
{
    process* proc = vmalloc(sizeof(process));
    memset(proc, 0, sizeof(process));
    proc->name = name;
    proc->pid = ++next_pid;
    proc->rip = addr;
    proc->state = PROCESS_STATE_ALIVE;
    proc->PML4T = pmm_calloc();
    uintptr_t* virt = PHYSICAL_TO_VIRTUAL(proc->PML4T); // Kernel space
    virt[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE;
    /*
    TEXT
    */
    uintptr_t phys = VIRTUAL_TO_PHYSICAL(addr);
    vmm_set_page(proc->PML4T, PROCESS_TEXT, phys, PAGE_USER | PAGE_PRESENT | PAGE_WRITE);

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
    proc->stack_top = PROCESS_STACK_TOP;
    proc->rsp = PROCESS_STACK_TOP - sizeof(task_register);
    virt = PHYSICAL_TO_VIRTUAL(((uint8_t*)phys) + 4096 - sizeof(task_register));
    task_register* stack = virt;
    stack->ss = 0x00;
    stack->rsp = proc->rsp;
    /*
    RFLAGS
    Bits	63..32	31	30	29	28	27	26	25	24	23	22	21	20	19	18	17	16	15	14	13..12	11	10	9	8	7	6	5	4	3	2	1	0
    Drapeaux	-	-	-	-	-	-	-	-	-	-	-	ID	VIP	VIF	AC	VM	RF	0	NT	IOPL	OF	DF	IF	TF	SF	ZF	0	AF	0	PF	1	CF
                0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   1   0   0   0   0   0   0   0   1   0
                    |           |   |           |   |           |   |           |   |           |   |           |   |           |   |           |

    Set trap flag and 1 flag (reserved)
    */
    stack->rflags = 0x00200246;
    stack->cs = 0x8;
    stack->rip = proc->rip;
    //stack->error_code = 0;
    //stack->interrupt_no = 0;
    stack->r15 = 0;
    stack->r14 = 0;
    stack->r13 = 0;
    stack->r12 = 0;
    stack->r11 = 0;
    stack->r10 = 0;
    stack->r9 = 0;
    stack->r8 = 0;
    stack->rbp = 0;
    stack->rdi = 0;
    stack->rsi = 0;
    stack->rdx = 0;
    stack->rcx = 0;
    stack->rbx = 0;
    stack->rax = 0;

    return proc;
}