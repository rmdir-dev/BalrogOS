#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/syscall/syscall.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"
#include <errno.h>
#include <string.h>

#include "balrog_os/cpu/state/cpu_state.h"

void sys_debug(interrupt_regs* stack_frame)
{
#ifdef KDB_DEBUG
    if(get_current_process()->uid != 0)
    {
        return;
    }

    set_debug_mode(stack_frame->rdi);
#endif
}
