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
#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/Debug/exception.h"
#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/Memory/kstack.h"
#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/User/user_manager.h"
#include "BalrogOS/CPU/FPU/fpu.h"

/* 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!  FOR TEST PURPOSE ONLY  !!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/*  we will have a loop that kill dead or zombie process
    this one is a place holder until that program is done.
*/
void idle_loop()
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

    /*     DEBUG OUTPUT   */
#ifdef KDB_DEBUG
#ifdef KDB_START_SEQ
#if KDB_START_SEQ == 1
    // show all debug messages at default level
    set_debug_mode(KDB_DEFAULT_LVL);
#elif KDB_START_SEQ == 0
    // show only critical errors
    set_debug_mode(3);
#endif
#endif
#endif

    /*      SCREEN        */
    vga_init();
    KERNEL_LOG_OK("Kernel loading :");
    KERNEL_LOG_OK("VGA Driver : done");

    /*   INTERRRUPTS      */
    KERNEL_LOG_INFO("Interrrupts : waiting...");
    init_interrupt();
    KERNEL_LOG_OK("Interrupt initialization : done");

    /*    EXCEPTIONS      */
    init_exception();
    KERNEL_LOG_INFO("Exception status: enabled");

    /*   FPU             */
    init_fpu();
    KERNEL_LOG_OK("FPU initialization : done");

    /*    SYSTEM CALL     */
    init_syscalls();
    KERNEL_LOG_OK("System calls initialization : done");

    /*    MEMORY          */
    // TODO later don't pass these as argument but fetch them using #define SMAP_PHYS_ADDR
    SMAP_entry* SMAPinfo = P2V(SMAP);
	uint16_t* SMAPsize = P2V(size);
    
    /*    Kernel Heap     */
    init_kheap(); // Kernel Logical
    
    /*    Virtual Memory  */
    init_vmm();
    KERNEL_LOG_OK("Virtual memory initialization : done");

    /*    Physical Memory */
    init_pmm(SMAPinfo, SMAPsize);
    KERNEL_LOG_OK("Physical memory initialization : done");

    /*    Kernel Heap    */
    init_vmheap();  // Kernel Virtual
    KERNEL_LOG_OK("Kernel heap initialization : done");

    /*    GDT and TSS    */
    init_gdt();
    KERNEL_LOG_OK("GDT and TSS : done");

    /*    PCI BUS        */
    init_pci();
    KERNEL_LOG_OK("PCI Bus : done");

    /*    SCHEDULER      */
    init_scheduler();
    KERNEL_LOG_OK("CPU scheduler initialization : done");

    /*    PROCESS        */
    init_process();

    /*    KEYBOARD       */
    init_keyboard();
    KERNEL_LOG_OK("Keyboard initialization : done");

    /*    FILE SYSTEM    */
    init_file_system();
    KERNEL_LOG_OK("File system initialization : done");

    /*    USER MANAGER   */
    init_user_manager();
    KERNEL_LOG_OK("User manager initialization : done");
    KERNEL_LOG_OK("Kernel initialization : done");
    KERNEL_LOG_OK("BalrogOS version : %s", __BALROG_VERSION__);

    /*    TEST PROCESS   */
    KERNEL_LOG_INFO("start process : waiting...");
    char test_arg1[11] = "/sbin/auth";
    char test_arg2[9] = "/boot/";
    uintptr_t argv[5] = { &test_arg1, &test_arg2, 0, 0, 0 };
    exec_process(argv[0], &argv, 0);
    push_process("morgoth", idle_loop, 0);

    KERNEL_LOG_OK("start CPU scheduler : done");
    KERNEL_LOG_OK("start process : done");

    /*   ENABLE DEBUG   */
#ifdef KDB_DEBUG
    set_debug_mode(KDB_DEFAULT_LVL);
#endif

    /*    ENABLE INTERRUPT   */
    kclear();

    enable_interrupt();
}