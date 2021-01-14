#include "Init.h"
#include "IO/tty/tty_io.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"
#include "CPU/Interrupts/irq.h"
#include "Drivers/Keyboard/keyboard.h"

void initialize_kernel()
{
    terminal_initialize();
    init_interrupt();
    init_irq();
    init_keyboard();
    asm("sti");
}