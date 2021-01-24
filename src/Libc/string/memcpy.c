#include "string.h"

void* memcpy(void* __restrict dest, const void* __restrict source, size_t size)
{
    unsigned char* dst = dest;
    const unsigned char* src = source;
    for(size_t i; i < size; i++)
    {
        dst[i] = src[i];
    }
    return dest;
}