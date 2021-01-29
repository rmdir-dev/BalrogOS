#include <string.h>

void* memset(void* pointer, int value, size_t count)
{
    unsigned char* ptr = pointer;

    for(size_t i = 0; i < count; i++)
    {
        ptr[i] = value;
    }

    return pointer;
}