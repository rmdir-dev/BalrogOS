#pragma once

#include <stddef.h>

#include "balrog_os/memory/memory.h"

static inline __attribute__((always_inline)) int user_ptr_ok(uintptr_t ptr)
{
    return ptr != 0 && ptr < KERNEL_MAP_BASE;
}

static inline __attribute__((always_inline)) int user_buf_ok(uintptr_t p, size_t len)
{
    return user_ptr_ok(p) && (p + len) > p && (p + len) <= KERNEL_MAP_BASE;
}