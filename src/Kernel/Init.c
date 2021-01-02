#include "Init.h"
#include "IO/tty/tty_io.h"
#include "Debug/debug_output.h"

void initialize_kernel()
{
    terminal_initialize();
}