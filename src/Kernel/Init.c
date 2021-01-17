#include "Init.h"
#include "Drivers/Screen/vga_driver.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"
#include "CPU/Interrupts/irq.h"
#include "Drivers/Keyboard/keyboard.h"
#include "Debug/debug_output.h"
#include "Memory/memory.h"

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

    uint64_t total_memory = 0;
    
    for(uint16_t i = 0; i < *SMAPsize; i++)
    {
        total_memory += SMAPinfo[i].Length;
        if(SMAPinfo[i].Length > ONE_MiB)
        {
            KERNEL_LOG_INFO("Base address : %x | Length %d MiB", SMAPinfo[i].BaseAddress, BYTE_TO_MiB(SMAPinfo[i].Length));
        } else 
        {
            KERNEL_LOG_INFO("Base address : %x | Length %d kiB", SMAPinfo[i].BaseAddress, BYTE_TO_KiB(SMAPinfo[i].Length));
        }
    }
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));

    /*     KEYBOARD     */
    init_keyboard();

    enable_interrupt();
}