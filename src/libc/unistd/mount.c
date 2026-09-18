#include <unistd.h>

int mount(const char* source, const char* target)
{
    asm volatile("mov %%rax, %%rdi": :"a"(source));
    asm volatile("mov %%rax, %%rsi": :"a"(target));
    asm volatile("mov $165, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}