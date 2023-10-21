#include <sys/debug.h>
#include "balrog/debug/debug.h"

void __sys_debug(int level)
{
    asm volatile("mov %%rax, %%rdi": :"a"(level));
    asm volatile("mov %%rax, %%rax": :"a"(KDB_CALL));
    asm volatile("int $0x80");
}
