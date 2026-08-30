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
#include "BalrogOS/Drivers/Serial/serial.h"

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
    while(1)
    {}
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

    /*
     * Serial & VGA cannot be logged as they're responsible for the outputs,
     * thus we recover their status and log afterward.
     */
    /*      SERIAL        */
    int serial_status = serial_init();

    /*      SCREEN        */
    int vga_status = vga_init();

    KERNEL_LOG_OK("Kernel loading :");
    KERNEL_LOG_RESULT(vga_status,    "VGA driver : ",    "done", "not initialized");
    KERNEL_LOG_RESULT(serial_status, "Serial driver : ", "done", "not initialized");

    /*   INTERRRUPTS      */
    KERNEL_LOG_ASSERT(init_interrupt(), "Interrupts : ", "done", "not initialized");

    /*    EXCEPTIONS      */
    KERNEL_LOG_ASSERT(init_exception(), "Exceptions : ", "enabled", "not enabled");

    /*   FPU              */
    KERNEL_LOG_ASSERT(init_fpu(), "FPU : ", "done", "not initialized");

    /*    SYSTEM CALL     */
    KERNEL_LOG_ASSERT(init_syscalls(), "System calls : ", "done", "not initialized");

    /*    MEMORY          */
    // TODO later don't pass these as argument but fetch them using #define SMAP_PHYS_ADDR
    SMAP_entry* SMAPinfo = P2V(SMAP);
	uint16_t* SMAPsize = P2V(size);

    /*    Kernel Heap     */
    KERNEL_LOG_ASSERT(init_kheap(), "Kernel logical heap : ", "done", "not initialized");

    /*    Virtual Memory  */
    KERNEL_LOG_ASSERT(init_vmm(), "Virtual memory : ", "done", "not initialized");

    /*    Physical Memory */
    KERNEL_LOG_ASSERT(init_pmm(SMAPinfo, SMAPsize), "Physical memory : ", "done", "not initialized");

    /*    Kernel Heap    */
    KERNEL_LOG_ASSERT(init_vmheap(), "Kernel virtual heap : ", "done", "not initialized");

    /*    GDT and TSS    */
    KERNEL_LOG_ASSERT(init_gdt(), "GDT and TSS : ", "done", "not initialized");

    /*    PCI BUS        */
    KERNEL_LOG_ASSERT(init_pci(), "PCI bus : ", "done", "not initialized");

    /*    SCHEDULER      */
    KERNEL_LOG_ASSERT(init_scheduler(), "CPU scheduler : ", "done", "not initialized");

    /*    PROCESS        */
    KERNEL_LOG_ASSERT(init_process(), "Process table : ", "done", "not initialized");

    /*    KEYBOARD       */
    KERNEL_LOG_ASSERT(init_keyboard(), "Keyboard : ", "done", "not initialized");

    /*    FILE SYSTEM    */
    KERNEL_LOG_ASSERT(init_file_system(), "File system : ", "done", "not mounted");

    /*    USER MANAGER   */
    KERNEL_LOG_ASSERT(init_user_manager(), "User manager : ", "done", "not initialized");

    KERNEL_LOG_OK("Kernel initialization : done");
    KERNEL_LOG_OK("BalrogOS version : %s", __BALROG_VERSION__);

    /*    TEST PROCESS   */
    KERNEL_LOG_INFO("start process : waiting...");
    char test_arg1[11] = "/sbin/auth";
    char test_arg2[9] = "/boot/";
    uintptr_t argv[5] = { &test_arg1, 0, 0, 0, 0 };
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