#include "balrog_os/cpu/scheduler/scheduler.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/cpu/tss/tss.h"
#include "balrog_os/cpu/gdt/gdt.h"
#include "balrog_os/cpu/rflags/rflag.h"
#include "balrog_os/cpu/pit/pit.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/cpu/apic/apic.h"
#include "klib/io/kprint.h"
#include <stddef.h>

extern process_list rdy_proc_list;
process* current_running = NULL;

extern tss_entry tss;

static inline __attribute__((always_inline)) void __suspend()
{
    asm volatile("push %rax");
    asm volatile("push %rbx");
    asm volatile("push %rcx");
    asm volatile("push %rdx");
    asm volatile("push %rsi");
    asm volatile("push %rdi");
    asm volatile("push %r8");
    asm volatile("push %r9");
    asm volatile("push %r10");
    asm volatile("push %r11");
    asm volatile("push %r12");
    asm volatile("push %r13");
    asm volatile("push %r14");
    asm volatile("push %r15");
    asm volatile("push %rbp");
    asm volatile("mov %%rsp, %%rax":"=a"(current_running->rsp));
    asm volatile("mov %%rbp, %%rax":"=a"(current_running->stack_top));
}

static inline __attribute__((always_inline)) void __resume()
{
    tss.rsp0 = current_running->kernel_stack_top;

    // Context restoration !!
    // DO NOT USE write_cr3 here it might break the restoration cycle.
    asm volatile("mov %%rax, %%cr3": :"a"(current_running->cr3));
    asm volatile("mov %%rax, %%rsp": :"a"(current_running->rsp));
    asm volatile("pop %rbp");
    asm volatile("pop %r15");
    asm volatile("pop %r14");
    asm volatile("pop %r13");
    asm volatile("pop %r12");
    asm volatile("pop %r11");
    asm volatile("pop %r10");
    asm volatile("pop %r9");
    asm volatile("pop %r8");
    asm volatile("pop %rdi");
    asm volatile("pop %rsi");
    asm volatile("pop %rdx");
    asm volatile("pop %rcx");
    asm volatile("pop %rbx");
    asm volatile("pop %rax");
}

static void __exec()
{
    // kernel_debug_output(KDB_LVL_VERBOSE, "scheduler : exec pid %d, exec %d, rsp 0%p, rip 0%p, cr3 0%p",
    //         current_running->pid, current_running->exec, current_running->rsp,
    //         current_running->rip, current_running->cr3);
    current_running->exec = 1;
    __resume();
    asm volatile("iretq");
}

static void __round_robin()
{
    __suspend();
    current_running = current_running->next;

    if(!current_running->exec)
    {
        __exec();
        return;
    }

    __resume();
}

void schedule(size_t tick, uint16_t ms)
{
    if(rdy_proc_list.head == NULL)
    {
        static int said = 0;

        if(!said)
        {
            said = 1;
            KERNEL_LOG_FAIL("scheduler : the ready list is empty, nothing left to run");
        }
        return;
    }

    if(current_running != NULL)
    {
        __round_robin();
        return;
    }

    current_running = rdy_proc_list.head;

    if (!current_running->exec)
    {
        __exec();
        return;
    }

    __resume();
}

/*  the same hundred ticks a second the pit was programmed for  */
#define SCHEDULER_HZ 100

int init_scheduler()
{
    if(lapic_timer_init(SCHEDULER_HZ, &schedule) == 0)
    {
        return 0;
    }

    // TODO set the pit speed faster to 10 000 or more
    init_pit(&schedule);

    return 0;
}


uintptr_t push_process(char* name, uintptr_t func, uint8_t mode)
{
    process* proc = create_process(name, func, mode);

    proc_insert_to_ready_queue(proc);

    kernel_debug_output(KDB_LVL_INFO, "scheduler : %s pid %d ready, entry 0%p, ring %d",
            name, proc->pid, func, mode);

    return proc->pid;
}