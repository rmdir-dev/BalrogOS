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
#include "BalrogOS/Memory/kstack.h"

/* 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!  FOR TEST PURPOSE ONLY  !!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#include <pthread.h>
#include <stdlib.h>

unsigned test_var = 0;
pthread_mutex_t mutex_test;

void test_init()
{
    pthread_mutex_init(&mutex_test, NULL);
}

char buf[4096 * 2];

void _test_print_dir(uint8_t* entires)
{
    fs_dir_entry* entry = entires;
    char* name = vmalloc(255);
    while(entry->inbr)
    {
        memcpy(name, &entry->name, entry->name_len);
        name[entry->name_len] = 0;
        kprint("%s \n", name);
        entires += entry->entry_size;
        entry = entires;
    }
}

void test_file()
{
    kprint("test file open \n");
    int fd = open("/", 001);
    buf[4096] = 0;
    kprint("test file read \n");
    read(fd, buf, 4096);
    _test_print_dir(buf);
    kprint("test file close \n");
    close(fd);

    exit(0);
    kprint("error exit don't work.\n");
}

void test_user_mode()
{
    while(1){}
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

    //uint8_t* kstack_test = kstack_alloc();
    //kprint("kstack addr = 0%p \n", kstack_test);
    //kstack_test -= 16;
    //*kstack_test = 225;
    //kprint("test = %d", *kstack_test);
    //kstack_free(kstack_test);
    //while(1){}

    /* GDT and TSS */
    init_gdt();
    KERNEL_LOG_OK("GDT and TSS : done");

    /* SCHEDULER */
    init_scheduler();
    KERNEL_LOG_OK("CPU scheduler initialization : done");

    /*     KEYBOARD     */
    init_keyboard();
    KERNEL_LOG_OK("Keyboard initialization : done");

    /* FILE SYSTEM */
    init_file_system();

    /* TEST PROCESS */
    test_init();
    //push_process("test", test2, 0);
    //push_process("test", test, 0);test_file
    push_process("test", test_file, 0);
    push_process("test", test_user_mode, 3);
    KERNEL_LOG_OK("start process");

    enable_interrupt();
}