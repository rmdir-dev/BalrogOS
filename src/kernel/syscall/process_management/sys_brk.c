#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"
#include "balrog/memory/proc_mem.h"
#include "balrog_os/debug/debug_output.h"

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