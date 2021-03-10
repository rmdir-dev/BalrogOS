#include "plib/park.h"
#include <stdio.h>

void setpark()
{
    //kprint("Park \n");
    asm volatile("mov $0, %rdi");
    asm volatile("mov $203, %rax");
    asm volatile("int $0x80");
}

void park()
{
    //kprint("Park \n");
    asm volatile("mov $0, %rdi");
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}

void unpark(uint64_t pid)
{
    //kprint("Unpark : %d \n", pid);
    asm volatile("mov %%rax, %%rdi": :"a"(pid));
    asm volatile("mov $202, %rax");
    asm volatile("int $0x80");
}