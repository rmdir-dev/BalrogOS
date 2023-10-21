#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Debug/debug_output.h"
#include <errno.h>
#include <string.h>

extern process* current_running;

void sys_debug(interrupt_regs* stack_frame)
{
#ifdef KDB_DEBUG
    if(current_running->uid != 0) {
        return;
    }

    set_debug_mode(stack_frame->rdi);
#endif
}