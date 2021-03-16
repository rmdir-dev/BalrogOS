#pragma once
#include <stdint.h>

/**
 * @brief 
 * 
 */
static inline void setpark()
{
    asm volatile("mov $0, %rdi");
    asm volatile("mov $203, %rax");
    asm volatile("int $0x80");
}

/**
 * @brief add the current process into the waiting queue
 * 
 */
static inline void park()
{
    asm volatile("mov $0, %rdi");
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}

/**
 * @brief wake a waiting process.
 * 
 * @param pid process ID of the process we want to wake up
 */
static inline void unpark(uint64_t pid)
{
    asm volatile("mov %%rax, %%rdi": :"a"(pid));
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}