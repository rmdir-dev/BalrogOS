#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "balrog/memory/proc_mem.h"
#include "BalrogOS/Debug/debug_output.h"

extern process* current_running;

int sys_brk(interrupt_regs* stack_frame)
{
    void* phys = pmm_calloc();
    kernel_debug_output(KDB_LVL_INFO, "brk : %p", phys);

    if(phys == 0 || stack_frame->rdi > PROCESS_HEAP_END)
    {
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "Alloc new page at : %p", stack_frame->rdi - 0x1000);
    vmm_set_page(current_running->PML4T, stack_frame->rdi - 0x1000, phys, PAGE_USER | PAGE_PRESENT | PAGE_WRITE);

    return 0;
}