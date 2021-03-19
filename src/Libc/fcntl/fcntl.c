#include <stdlib.h>
#include <fcntl.h>

size_t read(int fd, void* buf, size_t count)
{
    asm volatile("mov %%rax, %%rdi": :"a"(fd));
    asm volatile("mov %%rax, %%rsi": :"a"(buf));
    asm volatile("mov %%rax, %%rdx": :"a"(count));
    asm volatile("mov $0, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}

size_t write(int fd, void* buf, size_t count)
{
    asm volatile("mov %%rax, %%rdi": :"a"(fd));
    asm volatile("mov %%rax, %%rsi": :"a"(buf));
    asm volatile("mov %%rax, %%rdx": :"a"(count));
    asm volatile("mov $1, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}

int open(const char* pathname, int flags)
{
    asm volatile("mov %%rax, %%rdi": :"a"(pathname));
    asm volatile("mov %%rax, %%rsi": :"a"(flags));
    asm volatile("mov $2, %rax");
    int ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}

int close(int fd)
{
    asm volatile("mov %%rax, %%rdi": :"a"(fd));
    asm volatile("mov $3, %rax");
    int ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}