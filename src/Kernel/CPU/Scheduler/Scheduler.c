#include "BalrogOS/CPU/Scheduler/Scheduler.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Interrupts/irq.h"
#include "BalrogOS/Tasking/tasking.h"
#include <stdio.h>

process* process_queue = NULL;
process* last_process = NULL;
process* current_running = NULL;

uint8_t loop = 0;

static void _exec()
{
    current_running->exec = 1;
    asm volatile("mov %%rax, %%cr3": :"a"(current_running->cr3));
    asm volatile("mov %%rax, %%rsp": :"a"(current_running->rsp));
    asm volatile("mov %rbp, %rax");
    asm volatile("pop %rbp");
    asm volatile("mov %rax, %rbp");
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
    asm volatile("push %%rax": :"a"(current_running->stack_top));
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

/*
Will be use as temporary test scheduler.
*/
static interrupt_regs* round_robin_shedule(interrupt_regs* stack_frame)
{
    if(process_queue != NULL)
    {
        if(current_running != NULL)
        {
            
            _round_robin();
        } else 
        {
            
            current_running = process_queue;
            _exec();
        }
    } else 
    {
        printf("Schedule work! 0%x\n", stack_frame->rsp);
        irq_end(INT_IRQ_0);
    }
    return stack_frame;
}

void schedule()
{
    if(process_queue != NULL)
    {
        if(current_running != NULL)
        {
            _round_robin();
        } else 
        {
            current_running = process_queue;
            _exec();
        }
    } else 
    {
        printf("Schedule work!\n");
        irq_end(INT_IRQ_0);
    }
}

void init_scheduler()
{
    register_interrupt_handler(INT_IRQ_0, round_robin_shedule);
    irq_pic_toggle_mask_bit(INT_IRQ_0);
}


uintptr_t push_process(char* name, uintptr_t func)
{
    process* proc = create_process(name, func);
    
    if(process_queue == NULL)
    {
        process_queue = proc;
        proc->next = proc;
        proc->prev = proc;
    } else
    {
        proc->next = process_queue;
        last_process->next = proc;
        proc->prev = last_process;
    }

    last_process = proc;

    return proc->pid;
}