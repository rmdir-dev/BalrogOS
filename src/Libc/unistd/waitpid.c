#include <unistd.h>

pid_t waitpid(pid_t pid, int* status, int option)
{
    asm volatile("mov %%rax, %%rdi": :"a"(pid));
    asm volatile("mov %%rax, %%rsi": :"a"(status));
    asm volatile("mov %%rax, %%rdx": :"a"(option));
    asm volatile("mov $1, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}