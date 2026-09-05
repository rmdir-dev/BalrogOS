#include "errno.h"
#include "balrog_os/syscall/syscall.h"
#include "balrog_os/syscall/syscall_guard.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"
#include "balrog/memory/proc_mem.h"
#include "balrog_os/debug/debug_output.h"

extern process* current_running;

int sys_brk(interrupt_regs* stack_frame)
{
    uintptr_t requested =  PAGE_ALIGN_UP(stack_frame->rdi);

    if (!user_ptr_ok(requested) || requested > PROCESS_HEAP_END)
    {
        *current_running->error_no = EFAULT;
        return -1;
    }

    if (requested <= current_running->brk)
    {
        return 0;
    }

    for (uintptr_t new_page = current_running->brk; new_page < requested; new_page += PAGE_SIZE)
    {
        void* phys = pmm_calloc();

        if(!phys)
        {
            *current_running->error_no = ENOMEM;
            return -1;
        }

        vmm_set_page(current_running->PML4T, (void*) new_page, phys, PAGE_USER | PAGE_PRESENT | PAGE_WRITE);
    }

    current_running->brk = requested;
    kernel_debug_output(KDB_LVL_INFO, "brk : new heap top address %p", current_running->brk);

    return 0;
}