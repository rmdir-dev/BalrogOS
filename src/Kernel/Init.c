#include "Init.h"
#include "Drivers/Screen/vga_driver.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"
#include "CPU/Interrupts/irq.h"
#include "Drivers/Keyboard/keyboard.h"

void initialize_kernel()
{
    vga_init();
    init_interrupt();
    init_irq();
    init_keyboard();
    asm("sti");
}