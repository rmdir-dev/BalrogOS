#include "balrog/system/reboot.h"

#include <unistd.h>

int reboot(int option)
{
    asm volatile("mov %%rax, %%rdi": :"a"(BALROG_REBOOT_MAGIC1));
    asm volatile("mov %%rax, %%rsi": :"a"(BALROG_REBOOT_MAGIC2));
    asm volatile("mov %%rax, %%rdx": :"a"(option));
    asm volatile("mov $169, %rax");
    asm volatile("int $0x80");
    return 0;
}