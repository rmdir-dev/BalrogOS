#include <stdlib.h>

void exit(int code)
{
    asm volatile("mov %%rax, %%rdi": :"a"(code));
    asm volatile("mov $60, %rax");
    asm volatile("int $0x80");
}