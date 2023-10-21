#pragma once

#include "klib/IO/kprint.h"
#include "balrog/debug/debug.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#define _KERNEL_LOG_OK_MSG       "\e[0;97m[\e[0;92m  OK  \e[0m] "
#define _KERNEL_LOG_FAILURE_MSG  "\e[0;97m[\e[0;94mFAILED\e[0m] "
#define _KERNEL_LOG_INFO_MSG     "\e[0;97m[\e[0;97m INFO \e[0m] "

#define KERNEL_LOG_INFO(...) kprint(_KERNEL_LOG_INFO_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_OK(...) kprint(_KERNEL_LOG_OK_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_FAIL(...) kprint(_KERNEL_LOG_FAILURE_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG(...) kprint(__VA_ARGS__); kprint("\n")

#define KERNEL_DEBUG_MODE_OFF       3
#define KERNEL_DEBUG_MODE_ERROR     2
#define KERNEL_DEBUG_MODE_INFO      1

#define kernel_debug_output(level, ...) if(__kernel_debug_output(level) && kprint(__VA_ARGS__) && kprint("\n")){}

int __kernel_debug_output(int level);

void set_debug_mode(int mode);