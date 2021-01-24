#include "string.h"


int memcmp(const void* addr_1, const void* addr_2, size_t byte_size)
{
    const unsigned char* ptr_1 = addr_1;
    const unsigned char* ptr_2 = addr_2;
    for(size_t i; i < byte_size; i++)
    {
        if(ptr_1[i] < ptr_2[i])
        {
            return -1;
        } else if (ptr_1[i] > ptr_2[i])
        {
            return 1;
        }
    }

    return 0;
}