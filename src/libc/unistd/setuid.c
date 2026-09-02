void setuid(int uid) {
    asm volatile("mov %%rax, %%rdi": :"a"(uid));
    asm volatile("mov $105, %rax");
    asm volatile("int $0x80");
}