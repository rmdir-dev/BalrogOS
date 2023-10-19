char* getcwd(char* buf, int size) {
    char* ret = 0;
    asm volatile("mov %%rax, %%rdi": :"a"(buf));
    asm volatile("mov %%rax, %%rsi": :"a"(size));
    asm volatile("mov $79, %rax");
    asm volatile("int $0x80": "a="(ret));

    return ret;
}