#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/syscall/syscall_guard.h"
#include "balrog_os/tasking/tasking.h"
#include "klib/io/kprint.h"
#include "libc/string.h"

void* sys_getcwd(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();
    char* buf = (char*) stack_frame->rdi;
    size_t size = (size_t) stack_frame->rsi;

    if(!user_buf_ok((uintptr_t) buf, size))
    {
        return NULL;
    }

    if(!current_running->cwd)
    {
        return NULL;
    }

    if(size < strlen(current_running->cwd))
    {
        return NULL;
    }

    strcpy(buf, current_running->cwd);

    return buf;
}