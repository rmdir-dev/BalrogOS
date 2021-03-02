#include <string.h>
#include <stddef.h>
#include <stdint.h>

char* strcpy(void* __restrict dest, const void* __restrict source)
{
    size_t len = strlen(source);
    memcpy(dest, source, len);
    return 0;
}