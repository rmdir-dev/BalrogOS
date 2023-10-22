#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Tasking/process.h"
#include "balrog/terminal/term.h"

extern process* current_running;

static interrupt_regs* general_protection_fault(interrupt_regs* stack_frame)
{
    kprint(TERM_CLEAR);
    kernel_debug_output(KDB_LVL_CRITICAL, "General protection fault : ");
    uint64_t error = stack_frame->error_code;
    if(error & 0x1)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "internal error ");
    }
    switch ((error >> 1) & 0b11)
    {
    case 0b00:
        kernel_debug_output(KDB_LVL_CRITICAL, "GDT ");
        break;

    case 0b01:
        kernel_debug_output(KDB_LVL_CRITICAL, "IDT ");
        break;

    case 0b10:
        kernel_debug_output(KDB_LVL_CRITICAL, "LDT ");
        break;

    case 0b11:
       kernel_debug_output(KDB_LVL_CRITICAL, "IDT ");
        break;
    
    default:
        break;
    }
    kernel_debug_output(KDB_LVL_CRITICAL, "index : 0%x\n", ((error >> 3) & 0b1111111111111));
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

    kprint(TERM_CLEAR);
    kernel_debug_output(KDB_LVL_CRITICAL, "Proc %d ", current_running->pid);

    if(regs->error_code & 0x02)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "write from ", regs->error_code);
    } else if(regs->error_code & 0x0e)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "execute code from ", regs->error_code);
    } else 
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "read to ", regs->error_code);
    }

    if(regs->error_code & 0x04)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "user mode ");
    } else
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "kernel mode ");
    }

    if(regs->error_code & 0x01)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "PROTECTION FAULT 0%p ", address);
    } else
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "PAGE MISS 0%p ", address);
    }

    kernel_debug_output(KDB_LVL_CRITICAL, "0%x\n", regs->rax);
    kernel_debug_output(KDB_LVL_CRITICAL, "rip 0%p\n", regs->rip);
    while(1)
    {}
    
    return regs;
}

void init_exception()
{
    register_interrupt_handler(INT_GP, general_protection_fault);
    register_interrupt_handler(INT_PF, vmm_page_fault_handler);
}