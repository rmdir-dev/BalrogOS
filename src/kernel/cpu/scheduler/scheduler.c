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

#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/memory/kstack.h"
#include "klib/data_structure/queue.h"

queue_t kstack_to_clean;

static inline __attribute__((always_inline)) void __suspend(process* current_running)
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

static inline __attribute__((always_inline)) void __resume(process* current_running)
{
    arch_set_kernel_stack(current_running->kernel_stack_top);

    // Context restoration !!
    // DO NOT USE write_cr3 here it might break the restoration cycle.
    asm volatile(
            "mov %0, %%cr3\n"
            "mov %1, %%rsp"
            :
            : "r"(current_running->cr3), "r"(current_running->rsp)
            : "memory");
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

static void __exec(process* current_running)
{
    // kernel_debug_output(KDB_LVL_VERBOSE, "scheduler : exec pid %d, exec %d, rsp 0%p, rip 0%p, cr3 0%p",
    //         current_running->pid, current_running->exec, current_running->rsp,
    //         current_running->rip, current_running->cr3);
    current_running->exec = 1;
    __resume(current_running);
    asm volatile("iretq");
}

static void __round_robin(process* current_running)
{
    __suspend(current_running);
    current_running = current_running->next;
    set_current_process(current_running);

    if(!current_running->exec)
    {
        __exec(current_running);
        return;
    }

    __resume(current_running);
}

void gothmog_schedule(size_t tick, uint16_t ms)
{
    if(sched_get_ready_list()->head == NULL)
    {
        static int said = 0;

        if(!said)
        {
            said = 1;
            KERNEL_LOG_FAIL("scheduler : the ready list is empty, nothing left to run");
        }
        return;
    }

    process* current_running = get_current_process();

    if(current_running != NULL)
    {
        if (!queue_empty(&kstack_to_clean))
        {
            do
            {
                uintptr_t kstack;
                if (queue_dequeue(&kstack_to_clean, &kstack) != -1)
                {
                    kstack_free((uintptr_t*) kstack);
                }
            } while (!queue_empty(&kstack_to_clean));
        }

        __round_robin(current_running);
        return;
    }

    current_running = sched_get_ready_list()->head;
    set_current_process(current_running);

    if (!current_running->exec)
    {
        __exec(current_running);
        return;
    }

    __resume(current_running);
}

/*  the same hundred ticks a second the pit was programmed for  */
#define SCHEDULER_HZ 100

int init_scheduler()
{
    if(lapic_timer_init(SCHEDULER_HZ, &gothmog_schedule) == 0)
    {
        return 0;
    }

    // TODO set the pit speed faster to 10 000 or more
    init_pit(&gothmog_schedule);
    queue_init(&kstack_to_clean);

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