#include <stdint.h>

int getpid()
{
    int ret = 0;
    asm volatile("mov $39, %rax");
    asm volatile("int $0x80": "a="(ret));
    return ret;
}