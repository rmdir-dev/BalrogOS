#include "BalrogOS/CPU/Scheduler/Scheduler.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Interrupts/irq.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/CPU/TSS/tss.h"
#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/CPU/RFLAGS/rflag.h"
#include "BalrogOS/CPU/PIT/pit.h"
#include "BalrogOS/Tasking/process.h"
#include "klib/IO/kprint.h"
#include <stddef.h>

extern process_list rdy_proc_list;
process* current_running = NULL;

extern tss_entry tss;

static void _exec()
{
    current_running->exec = 1;
    tss.rsp0 = current_running->kernel_stack_top;
    //kprint("rsp : 0%p \n", tss.rsp0);
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

static void _round_robin()
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
    
    current_running = current_running->next;

    if(!current_running->exec)
    {
        _exec();
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

interrupt_regs* schedule(interrupt_regs* stack_frame)
{
    if(rdy_proc_list.head != NULL)
    {
        if(current_running != NULL)
        {
            _round_robin();
        } else 
        {
            current_running = rdy_proc_list.head;
            _exec();
        }
    } else 
    {
        //kprint("Schedule work!\n");
        irq_end(INT_IRQ_0);
    }
    return stack_frame;
}

void init_scheduler()
{
    irq_pic_toggle_mask_bit(INT_IRQ_0);
    register_interrupt_handler(INT_IRQ_0, schedule);
    init_pit(100);
}


uintptr_t push_process(char* name, uintptr_t func, uint8_t mode)
{
    process* proc = create_process(name, func, mode);
    
    proc_insert_to_ready_queue(proc);

    return proc->pid;
}