#include <unistd.h>

int execv(const char *name, char *const argv[])
{
    // Should never return !
    asm volatile("mov %%rax, %%rdi": :"a"(name));
    asm volatile("mov %%rax, %%rsi": :"a"(argv));
    asm volatile("mov %%rax, %%rdx": :"a"(0));
    asm volatile("mov $39, %rax");
    asm volatile("int $0x80");
    return -1;
}