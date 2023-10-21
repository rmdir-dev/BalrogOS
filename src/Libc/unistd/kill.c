#include <unistd.h>

int kill(pid_t pid, int sig)
{
    asm volatile("mov %%rax, %%rdi": :"a"(pid));
    asm volatile("mov %%rax, %%rsi": :"a"(sig));
    asm volatile("mov $62, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}