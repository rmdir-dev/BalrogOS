#include "klib/IO/kprint.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "BalrogOS/Debug/debug_output.h"
#include "balrog/terminal/term.h"

#define _KBD_VERBOSE_MSG    "\e[0;91mINFO \e[0m : "
#define _KBD_INFO_MSG       "\e[0;97mINFO \e[0m : "
#define _KBD_ERROR_MSG      "\e[0;96mERROR\e[0m : "
#define _KBD_CRITICAL_MSG   "\e[0;94mCRITICAL\e[0m : "

int debug_mode = 3;

int __kernel_debug_output(int level)
{
    if(debug_mode > level && level != KDB_LVL_CRITICAL) {
        return 0;
    }

    switch (level) {
        case KDB_LVL_VERBOSE:
            kprint(_KBD_VERBOSE_MSG);
            break;
        case KDB_LVL_INFO:
            kprint(_KBD_INFO_MSG);
            break;
        case KDB_LVL_ERROR:
            kprint(_KBD_ERROR_MSG);
            break;
        case KDB_LVL_CRITICAL:
            kprint(_KBD_CRITICAL_MSG);
            break;
    }

    return -1;
}

void set_debug_mode(int mode)
{
#ifdef KDB_DEBUG
    kernel_debug_output(KDB_LVL_INFO, "Debug mode set to %d", mode);
    debug_mode = mode;
#endif
}