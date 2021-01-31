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

void initialize_kernel(void* SMAP, void* size)
{
    disable_interrupt();
    /*      SCREEN      */
    vga_init();
    printf("Kernel loading : \n");
    KERNEL_LOG_OK("VGA Driver : done");

    /*   INTERRRUPTS    */
    KERNEL_LOG_INFO("Interrrupts : waiting...");
    init_interrupt();
    KERNEL_LOG_OK("Interrupt initialization : done");
    //init_irq();

    /*      MEMORY      */
    SMAP_entry* SMAPinfo = PHYSICAL_TO_VIRTUAL(SMAP);
	uint16_t* SMAPsize = PHYSICAL_TO_VIRTUAL(size);
    
    /* Virtual Memory */
    init_vmm();
    //KERNEL_LOG_OK("Virtual memory initialization : done");

    /* Physical Memory */
    init_pmm(SMAPinfo, SMAPsize);
    //KERNEL_LOG_OK("Physical memory initialization : done");

    /* Kernel Heap */
    init_kheap(); // Kernel Logical
    init_vmheap();  // Kernel Virtual
    //KERNEL_LOG_OK("Kernel heap initialization : done");

    KERNEL_LOG_INFO("vmalloc test : waiting...");
    uint8_t* new_var = vmalloc(4086);
    *new_var = 55;
    KERNEL_LOG_INFO("new var addr : %p", new_var);
    KERNEL_LOG_INFO("new var val : %d", *new_var);
    long* new_var2 = vmalloc(sizeof(long));
    *new_var2 = 7356;
    KERNEL_LOG_INFO("new var addr : %p", new_var2);
    KERNEL_LOG_INFO("new var val : %d", *new_var2);
    int* new_var3 = vmalloc(sizeof(int));
    *new_var3 = 756;
    KERNEL_LOG_INFO("new var addr : %p", new_var3);
    KERNEL_LOG_INFO("new var val : %d", *new_var3);
    vmfree(new_var2);
    vmfree(new_var);
    vmfree(new_var3);
    new_var3 = vmalloc(sizeof(int));
    *new_var3 = 523;
    KERNEL_LOG_INFO("new var addr : %p", new_var3);
    KERNEL_LOG_INFO("new var val : %d", *new_var3);
    KERNEL_LOG_OK("vmalloc test : done");

    /*     KEYBOARD     */
    init_keyboard();

    /* SCHEDULER */
    init_scheduler();

    enable_interrupt();
}