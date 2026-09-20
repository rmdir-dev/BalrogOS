#include "balrog_os/init.h"
#include "balrog_os/drivers/screen/vga_driver.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/drivers/keyboard/keyboard.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/cpu/scheduler/scheduler.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/debug/exception.h"
#include "balrog_os/cpu/gdt/gdt.h"
#include "balrog_os/syscall/syscall.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/memory/kstack.h"
#include "balrog_os/drivers/bus/pci.h"
#include "balrog_os/drivers/usb/xhci/xhci.h"
#include "balrog_os/user/user_manager.h"
#include "balrog_os/cpu/fpu/fpu.h"
#include "balrog_os/drivers/serial/serial.h"
#include "balrog_os/cpu/acpi/acpi.h"
#include "balrog_os/cpu/cpuid/cpuid.h"
#include "balrog_os/cpu/rtc/rtc.h"
#include "balrog_os/debug/klog.h"
#include "balrog_os/drivers/disk/ahci/ahci.h"
#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/file_system/pstore/pstore.h"
#include "balrog_os/drivers/screen/fb_backend.h"

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
    set_debug_mode(5);
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

    /*      SCREEN        */
    klog_claim_buffer();

    KERNEL_LOG_OK("Kernel loading :");
    KERNEL_LOG_RESULT(vga_status,    "VGA driver : ",    "done", "not initialized");
    KERNEL_LOG_RESULT(serial_status, "Serial driver : ", "done", "not initialized");

    fb_claim_memory();
    /*    CPU    */
    int ret_status = init_cpu_info();
    KERNEL_LOG_ASSERT(ret_status, "CPU identification : ", "done", "not available");

    /*    GDT and TSS    */
    KERNEL_LOG_ASSERT(init_gdt(), "GDT and TSS : ", "done", "not initialized");

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
    ret_status = init_kheap();
    KERNEL_LOG_ASSERT(ret_status, "Kernel logical heap : ", "done", "not initialized");

    /*    Virtual Memory  */
    KERNEL_LOG_ASSERT(init_vmm(), "Virtual memory : ", "done", "not initialized");

    /*    Physical Memory */
    pmm_disable_alloc_logs();
    ret_status = init_pmm(SMAPinfo, SMAPsize);
    KERNEL_LOG_ASSERT(ret_status, "Physical memory : ", "done", "not initialized");

    /*    Kernel Heap    */
    ret_status = init_vmheap();
    KERNEL_LOG_ASSERT(ret_status, "Kernel virtual heap : ", "done", "not initialized");
    pmm_enable_alloc_logs();

    /*    RTC            */
    KERNEL_LOG_ASSERT(init_rtc(), "rtc : ", "done", "not initialized");

    /*    KLOG           */
    KERNEL_LOG_ASSERT(init_klog(), "klog : ", "done", "not initialized");

    /*    PSTORE        */
    int pstore_ret = init_pstore();
    KERNEL_LOG_ASSERT(pstore_ret, "PSTORE : ", "done", "not initialized");

    /*    PCI BUS        */
    ret_status = init_pci();
    KERNEL_LOG_ASSERT(ret_status, "PCI bus : ", "done", "not initialized");

    /*    SCHEDULER      */
    ret_status = init_scheduler();
    KERNEL_LOG_ASSERT(ret_status, "CPU scheduler : ", "done", "not initialized");

    /*    PROCESS        */
    KERNEL_LOG_ASSERT(init_process(), "Process table : ", "done", "not initialized");

    /*    ACPI          */
    KERNEL_LOG_ASSERT(init_acpi(), "ACPI : ", "done", "not available");


    /*    ATA           */
    KERNEL_LOG_ASSERT(init_ata(), "ATA controller : ", "done", "not found");

    /*    AHCI          */
    KERNEL_LOG_ASSERT(init_ahci(), "AHCI controller : ", "done", "not found");

    /*    USB           */
    ret_status = init_xhci();
    KERNEL_LOG_ASSERT(ret_status, "XHCI controller : ", "done", "not found");

    /*    KEYBOARD       */
    KERNEL_LOG_ASSERT(init_keyboard(), "Keyboard : ", "done", "not initialized");

    /*    FILE SYSTEM    */
    ret_status = init_file_system();
    KERNEL_LOG_ASSERT(ret_status, "File system : ", "done", "not mounted");

    /*    USER MANAGER   */
    KERNEL_LOG_ASSERT(init_user_manager(), "User manager : ", "done", "not initialized");

    KERNEL_LOG_OK("Kernel initialization : done");
    KERNEL_LOG_OK("BalrogOS version : %s", __BALROG_VERSION__);

    /*    TEST PROCESS   */
    KERNEL_LOG_INFO("start process : waiting...");
    char test_arg1[12] = "/sbin/durin";
    char test_arg2[9] = "/boot/";
    uintptr_t argv[5] = { &test_arg1, 0, 0, 0, 0 };
    exec_process(argv[0], &argv, 0);
    push_process("morgoth", idle_loop, 0);
    push_process("wormtongue", wormtongue, 0);

    KERNEL_LOG_OK("start CPU scheduler : done");
    KERNEL_LOG_OK("start process : done");

    /*   ENABLE DEBUG   */
#ifdef KDB_DEBUG
    set_debug_mode(KDB_DEFAULT_LVL);
#endif

    /*    ENABLE INTERRUPT   */
    vga_clear();

    enable_interrupt();
}