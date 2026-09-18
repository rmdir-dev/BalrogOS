#pragma once

#include <stddef.h>

#include "balrog_os/memory/memory.h"

static inline int user_ptr_ok(uintptr_t ptr)
{
    return ptr != 0 && ptr < KERNEL_OFFSET;
}

static inline int user_buf_ok(uintptr_t p, size_t len)
{
    return user_ptr_ok(p) && (p + len) > p && (p + len) <= KERNEL_OFFSET;
}