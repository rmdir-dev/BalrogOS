int getuid() {
    int ret = 0;
    asm volatile("mov $102, %rax");
    asm volatile("int $0x80": "a="(ret));
    return ret;
}