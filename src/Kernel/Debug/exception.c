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

void init_exception()
{
    register_interrupt_handler(INT_GENERAL_PROTECTION_FAULT, general_protection_fault);
}