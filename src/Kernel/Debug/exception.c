#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

static interrupt_regs* general_protection_fault(interrupt_regs* stack_frame)
{
    kprint(_KERNEL_LOG_FAILURE_MSG);
    kprint("General protection fault : ");
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
    while(1){}
    return stack_frame;
}

static interrupt_regs* vmm_page_fault_handler(interrupt_regs* regs)
{
    kprint(_KERNEL_LOG_FAILURE_MSG);

    if(regs->error_code & 0x02)
    {
        kprint("read from ");
    } else if(regs->error_code & 0x0e)
    {
        kprint("execute code from ");
    } else 
    {
        kprint("write to ");
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
        kprint("PROTECTION FAULT ");
    } else 
    {
        kprint("PAGE MISS ");
    }

    kprint("0%x\n", regs->rax);
    
    while(1)
    {}
    
    return regs;
}

void init_exception()
{
    register_interrupt_handler(INT_GP, general_protection_fault);
    register_interrupt_handler(INT_PF, vmm_page_fault_handler);
}