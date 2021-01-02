#pragma once

/*
ADDRESSES
*/
#define KERNEL_OFFSET 0xFFFFFF8000000000

#include <stdint.h>

#define VIRTUAL_TO_PHYSICAL(addr) (uintptr_t)addr & ~KERNEL_OFFSET
#define PHYSICAL_TO_VIRTUAL(addr) (uintptr_t)addr | KERNEL_OFFSET