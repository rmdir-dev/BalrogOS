#pragma once

#include "klib/IO/kprint.h"
#include "balrog/debug/debug.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#define _KERNEL_LOG_OK_MSG       "\e[0;97m[\e[0;92m  OK  \e[0m] "
#define _KERNEL_LOG_FAILURE_MSG  "\e[0;97m[\e[0;94mFAILED\e[0m] "
#define _KERNEL_LOG_FATAL_MSG   "\e[0;97m[\e[0;94m FATAL \e[0m] "
#define _KERNEL_LOG_INFO_MSG     "\e[0;97m[\e[0;97m INFO \e[0m] "

#define KERNEL_LOG_INFO(...) kprint(_KERNEL_LOG_INFO_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_OK(...) kprint(_KERNEL_LOG_OK_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_FAIL(...) kprint(_KERNEL_LOG_FAILURE_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_FATAL(...) kprint(_KERNEL_LOG_FATAL_MSG); kprint(__VA_ARGS__); kprint("\n"); while(1){}
#define KERNEL_LOG(...) kprint(__VA_ARGS__); kprint("\n")

#define _KERNEL_LOG_RESET_LINE   "\r\e[K" // Reset the line with \r -> rewrite the line


#define KERNEL_LOG_RESULT(status, message, ok_out, fail_out)            \
      do {                                                              \
          if((status) == 0) { KERNEL_LOG_OK(message "%s", ok_out); }    \
          else              { KERNEL_LOG_FAIL(message "%s", fail_out); }\
      } while(0)

#define KERNEL_LOG_ASSERT(call, message, ok_out, fail_out) \
      do { \
          kprint(_KERNEL_LOG_INFO_MSG); kprint(message "%s", "waiting..."); \
          int __assert_ret = (call); \
          kprint(_KERNEL_LOG_RESET_LINE); \
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


#define KERNEL_DEBUG_MODE_OFF       3
#define KERNEL_DEBUG_MODE_ERROR     2
#define KERNEL_DEBUG_MODE_INFO      1

#ifndef KDB_DEFAULT_LVL
#define KDB_DEFAULT_LVL 3
#endif

#define kernel_debug_output(level, ...) if(__kernel_debug_output(level) && kprint(__VA_ARGS__) && kprint("\n")){}
#define kernel_debug_output_no_ln(level, ...) if(__kernel_debug_output(level) && kprint(__VA_ARGS__)){}

int __kernel_debug_output(int level);

void set_debug_mode(int mode);