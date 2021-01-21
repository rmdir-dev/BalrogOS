#include "Init.h"
#include "Drivers/Screen/vga_driver.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"
#include "CPU/Interrupts/irq.h"
#include "Drivers/Keyboard/keyboard.h"
#include "Debug/debug_output.h"
#include "Memory/memory.h"
#include "Memory/vmm.h"
#include "Memory/pmm.h"

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
    init_irq();

    /*      MEMORY      */
    SMAP_entry* SMAPinfo = PHYSICAL_TO_VIRTUAL(SMAP);
	uint16_t* SMAPsize = PHYSICAL_TO_VIRTUAL(size);
    
    /* Virtual Memory */
    vmm_init();

    /* Physical Memory */
    pmm_init(SMAPinfo, SMAPsize);

    uint64_t* test_ptr1 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr1);
    uint64_t* test_ptr2 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr2);
    uint64_t* test_ptr3 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr3);
    uint64_t* test_ptr4 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr4);
    pmm_free(test_ptr2);
    uint64_t* test_ptr5 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr5);
    pmm_free(test_ptr3);
    pmm_free(test_ptr4);
    uint64_t* test_ptr6 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr6);
    uint64_t* test_ptr7 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr7);
    uint64_t* test_ptr8 = pmm_calloc();
    KERNEL_LOG_INFO("%x", test_ptr8);


    /* Kernel Heap */

    /*     KEYBOARD     */
    init_keyboard();

    enable_interrupt();
}