#pragma once

#include "klib/io/kprint.h"
#include "balrog/debug/debug.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#define _KERNEL_LOG_OK_MSG       "\e[0;97m[\e[0;92m  OK  \e[0m] "
#define _KERNEL_LOG_FAILURE_MSG  "\e[0;97m[\e[0;94mFAILED\e[0m] "
#define _KERNEL_LOG_FATAL_MSG   "\e[0;97m[\e[0;94m FATAL \e[0m] "
#define _KERNEL_LOG_INFO_MSG     "\e[0;97m[\e[0;97m INFO \e[0m] "

#define KERNEL_LOG_INFO(...) __kernel_debug_output(KDB_NONE, 1, _KERNEL_LOG_INFO_MSG __VA_ARGS__)
#define KERNEL_LOG_OK(...) __kernel_debug_output(KDB_NONE, 1, _KERNEL_LOG_OK_MSG __VA_ARGS__)
#define KERNEL_LOG_FAIL(...) __kernel_debug_output(KDB_NONE, 1, _KERNEL_LOG_FAILURE_MSG __VA_ARGS__)
#define KERNEL_LOG_FATAL(...) kernel_debug_fatal(_KERNEL_LOG_FATAL_MSG __VA_ARGS__)

#define KERNEL_LOG_RESULT(status, message, ok_out, fail_out)            \
      do {                                                              \
          if((status) == 0) { KERNEL_LOG_OK(message "%s", ok_out); }    \
          else              { KERNEL_LOG_FAIL(message "%s", fail_out); }\
      } while(0)

#define KERNEL_LOG_ASSERT(call, message, ok_out, fail_out) \
      do { \
          __kernel_debug_output(KDB_NONE, 1, _KERNEL_LOG_INFO_MSG message "waiting..."); \
          int __assert_ret = (call); \
          if(__assert_ret == 0) { KERNEL_LOG_OK(message "%s", ok_out); } \
          else                  { KERNEL_LOG_FAIL(message "%s", fail_out); } \
      } while(0)

#ifdef KDB_DEBUG
#define KERNEL_DEBUG_EXEC(call) \
    do { \
        call \
    } while(0)
#else
#define KERNEL_DEBUG_EXEC(call)
#endif


#define KERNEL_DEBUG_MODE_OFF       5
#define KERNEL_DEBUG_MODE_ERROR     4
#define KERNEL_DEBUG_MODE_INFO      3

#ifndef KDB_DEFAULT_LVL
#define KDB_DEFAULT_LVL 5
#endif

#ifdef KDB_DEBUG

#define kernel_debug_output(level, ...)         __kernel_debug_output(level, 1, __VA_ARGS__)
#define kernel_debug_output_no_ln(level, ...)   __kernel_debug_output(level, 0, __VA_ARGS__)
#define kernel_debug_fatal(...)                 __kernel_debug_output(KDB_LVL_FATAL, 1, __VA_ARGS__)

#else

#define kernel_debug_output(level, ...) if((level) >= KDB_LVL_ERROR && __kernel_debug_output(level, 1, __VA_ARGS__)) {}
#define kernel_debug_output_no_ln(level, ...) if((level) >= KDB_LVL_ERROR && __kernel_debug_output(level, 0, __VA_ARGS__)) {}
#define kernel_debug_fatal(...)                 __kernel_debug_output(KDB_LVL_FATAL, 1, __VA_ARGS__)

#endif

void __kernel_debug_output(enum klog_logging_level level, int next_line, const char* __restrict format, ...);

void set_debug_mode(int mode);