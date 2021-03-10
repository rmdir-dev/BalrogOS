#pragma once

#include "lib/IO/kprint.h"

#define _KERNEL_LOG_OK_MSG       "\e[0;97m[\e[0;92m  OK  \e[0m] "
#define _KERNEL_LOG_FAILURE_MSG  "\e[0;97m[\e[0;94mFAILED\e[0m] "
#define _KERNEL_LOG_INFO_MSG     "\e[0;97m[\e[0;97m INFO \e[0m] "

#define KERNEL_LOG_OK(...) kprint(_KERNEL_LOG_OK_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_FAIL(...) kprint(_KERNEL_LOG_FAILURE_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG_INFO(...) kprint(_KERNEL_LOG_INFO_MSG); kprint(__VA_ARGS__); kprint("\n")
#define KERNEL_LOG(...) kprint(__VA_ARGS__); kprint("\n")