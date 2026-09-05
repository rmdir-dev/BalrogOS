#include "klib/io/kprint.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "balrog_os/debug/debug_output.h"
#include "balrog/terminal/term.h"
#include "balrog_os/drivers/serial/serial.h"

#define _KBD_VERBOSE_MSG    "\e[0;91mINFO \e[0m : "
#define _KBD_INFO_MSG       "\e[0;97mINFO \e[0m : "
#define _KBD_ERROR_MSG      "\e[0;96mERROR\e[0m : "
#define _KBD_CRITICAL_MSG   "\e[0;94mCRITICAL\e[0m : "

int debug_mode = KDB_DEFAULT_LVL;

int __kernel_debug_output(int level)
{
    int debug_only = debug_mode > level && level != KDB_LVL_CRITICAL;
    const char* message = "";

    switch (level) {
        case KDB_LVL_VERBOSE:
            message = _KBD_VERBOSE_MSG;
            break;
        case KDB_LVL_INFO:
            message = _KBD_INFO_MSG;
            break;
        case KDB_LVL_ERROR:
            message = _KBD_ERROR_MSG;
            break;
        case KDB_LVL_CRITICAL:
            message = _KBD_CRITICAL_MSG;
            break;
    }

    if (debug_only)
    {
        kdbprint(message);
        return 0;
    }

    kprint(message);
    return -1;
}

void set_debug_mode(int mode)
{
#ifdef KDB_DEBUG
//    kernel_debug_output(KDB_LVL_INFO, "Debug mode set to %d", mode);
    debug_mode = mode;
#endif
}