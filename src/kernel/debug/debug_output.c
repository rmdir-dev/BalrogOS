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
#include "balrog_os/memory/kheap.h"

#define _KDB_VERBOSE_MSG    "\e[0;97m[\e[0;90mVERBOSE \e[0m] "
#define _KDB_INFO_MSG       "\e[0;97m[\e[0;97m  INFO  \e[0m] "
#define _KDB_WARNING_MSG    "\e[0;97m[\e[0;96mWARNING \e[0m] "
#define _KDB_ERROR_MSG      "\e[0;97m[\e[0;94m ERROR  \e[0m] "
#define _KDB_CRITICAL_MSG   "\e[0;97m[\e[0;34mCRITICAL\e[0m] "
#define _KDB_FATAL_MSG      "\e[0;97m[\e[0;96m\e[0;44m FATAL  \e[0m] "

#define KDB_LINE_MAX 512

int debug_mode = KDB_DEFAULT_LVL;

extern int __ksprint(char* out, size_t maxsize, const char* format, va_list parameters);
extern int __print_string(const char* str, size_t size, enum klog_logging_level log_level);

void __kernel_debug_output(enum klog_logging_level level, int next_line, const char* __restrict format, ...)
{
    int debug_only = debug_mode > level && level != KDB_LVL_CRITICAL;
    const char* message = "";

    switch (level) {
        case KDB_LVL_VERBOSE:
            message = _KDB_VERBOSE_MSG;
            break;
        case KDB_LVL_INFO:
            message = _KDB_INFO_MSG;
            break;
        case KDB_LVL_WARNING:
            message = _KDB_WARNING_MSG;
            break;
        case KDB_LVL_ERROR:
            message = _KDB_ERROR_MSG;
            break;
        case KDB_LVL_CRITICAL:
            message = _KDB_CRITICAL_MSG;
            break;
        case KDB_LVL_FATAL:
            message = _KDB_FATAL_MSG;
            break;
    }

    char str[KDB_LINE_MAX];
    int pos = __ksprint(str, KDB_LINE_MAX, message, 0);
    va_list parameters;
    va_start(parameters, format);
    pos += __ksprint(str + pos, KDB_LINE_MAX - pos, format, parameters);
    va_end(parameters);

    if (next_line != 0)
    {
        pos += __ksprint(str + pos, KDB_LINE_MAX - pos, "\n", 0);
    }

    __print_string(str, pos, level);

    if (level == KDB_LVL_FATAL)
    {
        // force
        const char* fatal_msg = "FATAL ERROR: kdb : fatal error rebooting system !";
        __print_string(fatal_msg, strlen(fatal_msg), KDB_LVL_FATAL);
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