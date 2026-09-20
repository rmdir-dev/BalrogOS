#pragma once
#include <stdint.h>

/**
 * @brief 
 * 
 */
static inline void setpark()
{
    uint64_t call = 203;
    asm volatile("int $0x80" : "+a"(call) : "D"(0UL) : "memory");
}

/**
 * @brief add the current process into the waiting queue
 * 
 */
static inline void park()
{
    uint64_t call = 202;
    asm volatile("int $0x80" : "+a"(call) : "D"(0UL) : "memory");
}

/**
 * @brief wake a waiting process.
 * 
 * @param pid process ID of the process we want to wake up
 */
static inline void unpark(int pid)
{
    uint64_t call = 202;
    asm volatile("int $0x80" : "+a"(call) : "D"((uint64_t) pid) : "memory");
}