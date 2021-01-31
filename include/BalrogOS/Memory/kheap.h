#pragma once
#include <stddef.h>

void init_vmheap();

void* vmalloc(size_t size);

void vmfree(void* ptr);

void init_kheap();

void* kmalloc(size_t size);

void kfree(void* ptr);