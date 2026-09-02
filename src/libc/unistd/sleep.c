//
// Created by rmdir on 10/21/23.
//

#include <unistd.h>
#include "balrog/time/time.h"

int sleep(size_t ms) {
    timespec time;
    time.sec = ms / 1000;
    time.msec = ms % 1000;
    timespec ret_time = { 0, 0 };

    asm volatile("mov %%rax, %%rdi": :"a"(&time));
    asm volatile("mov %%rax, %%rsi": :"a"(&ret_time));
    asm volatile("mov $35, %rax");
    size_t ret = 0;
    asm volatile("int $0x80": "=a"(ret));

    return ret;
}