#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Tasking/process.h"
#include "balrog/terminal/term.h"

extern process* current_running;

static interrupt_regs* general_protection_fault(interrupt_regs* stack_frame)
{
//    kprint(TERM_CLEAR);
    kernel_debug_output_no_ln(KDB_LVL_CRITICAL, "General protection fault : ");
    uint64_t error = stack_frame->error_code;
    if(error & 0x1)
    {
        kprint("internal error ");
    }
    switch ((error >> 1) & 0b11)
    {
    case 0b00:
        kprint("GDT ");
        break;

    case 0b01:
        kprint("IDT ");
        break;

    case 0b10:
        kprint("LDT ");
        break;

    case 0b11:
       kprint("IDT ");
        break;
    
    default:
        break;
    }
    kprint("index : 0%x\n", ((error >> 3) & 0b1111111111111));
    kernel_debug_output(KDB_LVL_CRITICAL, "rip 0%p", stack_frame->rip);

//    if(current_running) {
//        kernel_debug_output(KDB_LVL_CRITICAL, "Term Proc %d", current_running->pid);
//        proc_kill(current_running, 1);
//        return stack_frame;
//    }

    while(1){}
    return stack_frame;
}

static interrupt_regs* vmm_page_fault_handler(interrupt_regs* regs)
{
    // get the value of the CR2 register
    // as the address register contain the address that cause the page fault
    uintptr_t address;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    // protection fault in user mode on forked process
    if(current_running && (regs->error_code & 0x04 || regs->error_code & 0x01) && current_running->forked_memory) {
        kernel_debug_output(KDB_LVL_INFO, "forked process memory protection fault -> copy parent memory \n");
        uintptr_t fault_base_address = address & 0xfffffffffffff000;
        copy_forked_process_memory(current_running, fault_base_address);
        return regs;
    }

//    kprint(TERM_CLEAR);
    kernel_debug_output_no_ln(KDB_LVL_CRITICAL, "Proc %d ", current_running->pid);

    if(regs->error_code & 0x02)
    {
        kprint("write from ", regs->error_code);
    } else if(regs->error_code & 0x0e)
    {
        kprint("execute code from ", regs->error_code);
    } else 
    {
        kprint("read to ", regs->error_code);
    }

    if(regs->error_code & 0x04)
    {
        kprint("user mode ");
    } else
    {
        kprint("kernel mode ");
    }

    if(regs->error_code & 0x01)
    {
        kprint("PROTECTION FAULT 0%p ", address);
    } else
    {
        kprint("PAGE MISS 0%p ", address);
    }

    kprint("0%x\n", regs->rax);
    kernel_debug_output_no_ln(KDB_LVL_CRITICAL, "rip 0%p", regs->rip);
    while(1)
    {}
    
    return regs;
}

void init_exception()
{
    register_interrupt_handler(INT_GP, general_protection_fault);
    register_interrupt_handler(INT_PF, vmm_page_fault_handler);
}