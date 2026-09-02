#include <unistd.h>

pid_t fork()
{
    asm volatile("mov $57, %rax");
    pid_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}