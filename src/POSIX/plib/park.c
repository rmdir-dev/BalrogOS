#include "plib/park.h"

void setpark()
{
    asm volatile("mov $0, %rdi");
    asm volatile("mov $203, %rax");
    asm volatile("int $0x80");
}

void park()
{
    asm volatile("mov $0, %rdi");
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}

void unpark(uint64_t pid)
{
    asm volatile("mov %%rax, %%rdi": :"a"(pid));
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}