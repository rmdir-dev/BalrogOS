#pragma once
#include <stdint.h>

/**
 * @brief execute an atomic test and set
 * 
 * @param addr the address of the variable you want to atomically test and set
 * @param new_val the new value of said variable.
 * @return uint64_t the old value contained into *addr
 */
static inline uint64_t xchg(volatile uint64_t* addr, uint64_t new_val)
{
    uint64_t ret = 0;
    asm volatile("lock\n"
                "xchgq %0, %1"
                : "+m"(*addr), "=a"(ret)    // OUTPUT
                : "1"(new_val)              // INPUT
                : "cc");                    // 
    return ret;
}