#include "klib/io/kprint.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "balrog_os/debug/debug_output.h"
#include "balrog/terminal/term.h"
#include "balrog_os/drivers/serial/serial.h"
#include "balrog_os/cpu/acpi/acpi.h"
#include "balrog_os/debug/klog.h"

#define _KBD_VERBOSE_MSG    "\e[0;91mVERBOSE\e[0m : "
#define _KBD_INFO_MSG       "\e[0;97mINFO \e[0m : "
#define _KDB_WARNING_MSG    "\e[0;96mWARNING\e[0m : "
#define _KBD_ERROR_MSG      "\e[0;96mERROR\e[0m : "
#define _KBD_CRITICAL_MSG   "\e[0;94mCRITICAL\e[0m : "
#define _KBD_FATAL_MSG      "\e[0;94mFATAL\e[0m : "

int debug_mode = KDB_DEFAULT_LVL;

extern int __kernel_print(const char* format, va_list parameters, enum klog_logging_level log_level);

void __kernel_debug_output(enum klog_logging_level level, int next_line, const char* __restrict format, ...)
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
        case KDB_LVL_WARNING:
            message = _KDB_WARNING_MSG;
            break;
        case KDB_LVL_ERROR:
            message = _KBD_ERROR_MSG;
            break;
        case KDB_LVL_CRITICAL:
            message = _KBD_CRITICAL_MSG;
            break;
        case KDB_LVL_FATAL:
            message = _KBD_FATAL_MSG;
            break;
    }

    // force print with critical.
    __kernel_print(message, 0, debug_only ? level : KDB_LVL_CRITICAL);
    va_list parameters;
    va_start(parameters, format);
    __kernel_print(format, parameters, level);
    va_end(parameters);

    if (next_line != 0)
    {
        __kernel_print("\n", 0, debug_only ? level : KDB_LVL_CRITICAL);
    }

    if (level == KDB_LVL_FATAL)
    {
        // force
        __kernel_print(message, 0, KDB_LVL_FATAL);
        __kernel_print("kdb : fatal error rebooting system !", 0, KDB_LVL_FATAL);
        __kernel_print("\n", 0, KDB_LVL_FATAL);
        // force flush
        klog_force_flush_buffers();

        // TODO :  add sleep time before shutdown (to be able to read the log).
        acpi_reboot();

        // if reboot failed -> freeze
        asm volatile("cli");
        while(1)
        {
            asm volatile("hlt");
        }
    }
}

void set_debug_mode(int mode)
{
#ifdef KDB_DEBUG
//    kernel_debug_output(KDB_LVL_INFO, "Debug mode set to %d", mode);
    debug_mode = mode;
#endif
}