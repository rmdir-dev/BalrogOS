#pragma once

#define _KERNEL_LOG_OK_MSG       "\e[0;97m[\e[0;92m  OK  \e[0m] "
#define _KERNEL_LOG_FAILURE_MSG  "\e[0;97m[\e[0;94mFAILED\e[0m] "
#define _KERNEL_LOG_INFO_MSG     "\e[0;97m[\e[0;97m INFO \e[0m] "

#define KERNEL_LOG_OK(...) printf(_KERNEL_LOG_OK_MSG); printf(__VA_ARGS__)
#define KERNEL_LOG_FAIL(...) printf(_KERNEL_LOG_FAILURE_MSG); printf(__VA_ARGS__)
#define KERNEL_LOG_INFO(...) printf(_KERNEL_LOG_INFO_MSG); printf(__VA_ARGS__)
#define KERNEL_LOG(...) printf(__VA_ARGS__)