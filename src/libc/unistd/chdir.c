int chdir(char* dir) {
    int ret = 0;
    asm volatile("mov %%rax, %%rdi": :"a"(dir));
    asm volatile("mov $80, %rax");
    asm volatile("int $0x80": "a="(ret));

    return ret;
}