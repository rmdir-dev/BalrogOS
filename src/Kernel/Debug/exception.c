#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

static interrupt_regs* general_protection_fault(interrupt_regs* stack_frame)
{
    printf(_KERNEL_LOG_FAILURE_MSG);
    printf("General protection fault : ");
    uint64_t error = stack_frame->error_code;
    if(error & 0x1)
    {
        printf("internal error ");
    }
    switch ((error >> 1) & 0b11)
    {
    case 0b00:
        printf("GDT ");
        break;

    case 0b01:
        printf("IDT ");
        break;

    case 0b10:
        printf("LDT ");
        break;

    case 0b11:
       printf("IDT ");
        break;
    
    default:
        break;
    }
    printf("index : 0%x\n", ((error >> 3) & 0b1111111111111));
    while(1){}
    return stack_frame;
}

static interrupt_regs* vmm_page_fault_handler(interrupt_regs* regs)
{
    printf(_KERNEL_LOG_FAILURE_MSG);

    if(regs->error_code & 0x02)
    {
        printf("read from ");
    } else if(regs->error_code & 0x0e)
    {
        printf("execute code from ");
    } else 
    {
        printf("write to ");
    }

    if(regs->error_code & 0x04)
    {
        printf("user mode ");
    } else
    {
        printf("kernel mode ");
    }

    if(regs->error_code & 0x01)
    {
        printf("PROTECTION FAULT ");
    } else 
    {
        printf("PAGE MISS ");
    }

    printf("0%x\n", regs->rax);
    
    while(1)
    {}
    
    return regs;
}

void init_exception()
{
    register_interrupt_handler(INT_GP, general_protection_fault);
    register_interrupt_handler(INT_PF, vmm_page_fault_handler);
}