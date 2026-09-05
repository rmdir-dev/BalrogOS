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
#include "klib/io/kprint.h"
#include <stddef.h>

extern process_list rdy_proc_list;
process* current_running = NULL;

extern tss_entry tss;

static void __exec()
{
    current_running->exec = 1;
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
    asm volatile("out %%al, %%dx": :"d"(0x20), "a"(0x20));
    asm volatile("pop %rdx");
    asm volatile("pop %rcx");
    asm volatile("pop %rbx");
    asm volatile("pop %rax");
    asm volatile("iretq");
}

static void __round_robin()
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

    current_running = current_running->next;

    if(!current_running->exec)
    {
        __exec();
        return;
    }

    tss.rsp0 = current_running->kernel_stack_top;

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
    asm volatile("out %%al, %%dx": :"d"(0x20), "a"(0x20));
    asm volatile("pop %rdx");
    asm volatile("pop %rcx");
    asm volatile("pop %rbx");
    asm volatile("pop %rax");
}

void schedule(size_t tick, uint16_t ms)
{
    if(rdy_proc_list.head == NULL)
    {
        return;
    }

    if(current_running != NULL)
    {
        __round_robin();
    } else
    {
        current_running = rdy_proc_list.head;
        __exec();
    }
}

int init_scheduler()
{
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