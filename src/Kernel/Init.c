#include "BalrogOS/Init.h"
#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Interrupts/irq.h"
#include "BalrogOS/Drivers/Keyboard/keyboard.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/CPU/Scheduler/Scheduler.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/CPU/GDT/gdt.h"

void test()
{
    while(1)
    {
        asm volatile("cli");
        printf("test\n");
        asm volatile("sti");
        for(uint64_t i = 0; i < 10000000; i++)
        {
        }
        asm volatile("cli");
        printf("and loop now! :D\n");
        asm volatile("sti");
    }
}

void test2()
{
    uint64_t test = 0;
    while(1)
    {
        asm volatile("cli");
        printf("something else %d | 0%p\n", test++, &test);
        asm volatile("sti");
        for(uint64_t i = 0; i < 100000000; i++)
        {  
        }
        asm volatile("cli");
        printf("hey I'm there\n");
        asm volatile("sti");
    }
}

void initialize_kernel(void* SMAP, void* size)
{
    disable_interrupt();
    /*      SCREEN      */
    vga_init();
    KERNEL_LOG_OK("Kernel loading :");
    KERNEL_LOG_OK("VGA Driver : done");

    /*   INTERRRUPTS    */
    KERNEL_LOG_INFO("Interrrupts : waiting...");
    init_interrupt();
    KERNEL_LOG_OK("Interrupt initialization : done");

    init_exception();

    /*      MEMORY      */
    SMAP_entry* SMAPinfo = PHYSICAL_TO_VIRTUAL(SMAP);
	uint16_t* SMAPsize = PHYSICAL_TO_VIRTUAL(size);
    
    /* Virtual Memory */
    init_vmm();
    KERNEL_LOG_OK("Virtual memory initialization : done");

    /* Physical Memory */
    init_pmm(SMAPinfo, SMAPsize);
    KERNEL_LOG_OK("Physical memory initialization : done");

    /* Kernel Heap */
    init_kheap(); // Kernel Logical
    init_vmheap();  // Kernel Virtual
    KERNEL_LOG_OK("Kernel heap initialization : done");

    /* SCHEDULER */
    init_scheduler();

    /*     KEYBOARD     */
    init_keyboard();

    push_process("test", test);
    push_process("test2", test2);
    //push_process("test2", test2);
    //KERNEL_LOG_OK("test process no fault");

    init_gdt();

    enable_interrupt();
}