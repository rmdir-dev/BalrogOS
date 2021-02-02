#pragma once
#include <stdint.h>

void init_scheduler();

uintptr_t push_process(char* name, uintptr_t func);