#include "Init.h"
#include "Drivers/Screen/vga_driver.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"
#include "CPU/Interrupts/irq.h"
#include "Drivers/Keyboard/keyboard.h"
#include "Debug/debug_output.h"

void initialize_kernel(SMAP_entry* SMAP, int16_t* size)
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
    uint64_t total_memory = 0;
    for(uint16_t i = 0; i < *size; i++)
    {
        total_memory += SMAP[i].Length;
        if(SMAP[i].Length > ONE_MiB)
        {
            KERNEL_LOG_INFO("Base address : %x | Length %d MiB", SMAP[i].BaseAddress, BYTE_TO_MiB(SMAP[i].Length));
        } else 
        {
            KERNEL_LOG_INFO("Base address : %x | Length %d kiB", SMAP[i].BaseAddress, BYTE_TO_KiB(SMAP[i].Length));
        }
    }
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));

    /*     KEYBOARD     */
    init_keyboard();

    enable_interrupt();
}