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
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/FileSystem/filesystem.h"

/* 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!  FOR TEST PURPOSE ONLY  !!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#include <pthread.h>

unsigned test_var = 0;
pthread_mutex_t mutex_test;

void test_init()
{
    pthread_mutex_init(&mutex_test, NULL);
}

void test()
{
    while(1)
    {
        //asm volatile("cli");
        //kprint("test\n");
        //asm volatile("sti");
        pthread_mutex_lock(&mutex_test);
        for(uint64_t i = 0; i < 100000000; i++)
        {
        }
        test_var++;
        //asm volatile("cli");
        kprint("and loop now! :D %d \n", test_var);
        //asm volatile("sti");
        pthread_mutex_unlock(&mutex_test);
    }
}

void test2()
{
    while(1)
    {
        //asm volatile("cli");
        //kprint("test2\n");
        //asm volatile("sti");
        for(uint64_t i = 0; i < 100000000; i++)
        {
        }
        pthread_mutex_lock(&mutex_test);
        test_var++;
        //asm volatile("cli");
        kprint("TEST 2 NOW :D %d \n", test_var);
        //asm volatile("sti");
        pthread_mutex_unlock(&mutex_test);
    }
}

void test_user_mode()
{
    uint64_t test = 0;
    char test_str[14] = "test syscall\n";

    while(1)
    {
        asm volatile("mov %%rax, %%rsi": :"a"(test_str));
        asm volatile("mov %%rax, %%rdx": :"a"(13));
        asm volatile("mov $1, %rax");
        asm volatile("int $0x80");
        for(uint64_t i = 0; i < 100000000; i++)
        {
        }
        test++;
    }
}

/* 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!  END OF TEST  !!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

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

    /* SYSTEM CALL */
    init_syscalls();

    /*      MEMORY      */
    // TODO later don't pass these as argument but fetch them using #define SMAP_PHYS_ADDR
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

    /* GDT and TSS */
    init_gdt();
    KERNEL_LOG_OK("GDT and TSS : done");

    /* SCHEDULER */
    init_scheduler();
    KERNEL_LOG_OK("CPU scheduler initialization : done");

    /*     KEYBOARD     */
    init_keyboard();
    KERNEL_LOG_OK("Keyboard initialization : done");

    /* TEST PROCESS */
    test_init();
    //push_process("test", test2, 0);
    //push_process("test", test, 0);
    //push_process("test", test_user_mode, 3);

    init_file_system();

    enable_interrupt();
}