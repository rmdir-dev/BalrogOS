#include <stat.h>

int fstat(int fd, void* buf)
{
    asm volatile("mov %%rax, %%rdi": :"a"(fd));
    asm volatile("mov %%rax, %%rsi": :"a"(buf));
    asm volatile("mov $5, %rax");
    asm volatile("int $0x80");
}