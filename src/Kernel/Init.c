#include "Init.h"
#include "IO/tty/tty_io.h"
#include "Debug/debug_output.h"
#include "CPU/Interrupts/interrupt.h"

void initialize_kernel()
{
    terminal_initialize();
    init_interrupt();
    asm("sti");
}