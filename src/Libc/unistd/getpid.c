#include <stdint.h>
#include <unistd.h>

pid_t getpid()
{
    pid_t ret = 0;
    asm volatile("mov $39, %rax");
    asm volatile("int $0x80": "a="(ret));
    return ret;
}