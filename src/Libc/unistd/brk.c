#include <unistd.h>

int brk(void* addr)
{
    asm volatile("mov %%rax, %%rdi": :"a"(addr));
    asm volatile("mov $12, %rax");
    int ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}