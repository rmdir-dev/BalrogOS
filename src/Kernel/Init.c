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

    /* Kernel Heap */

    /*     KEYBOARD     */
    init_keyboard();

    enable_interrupt();
}