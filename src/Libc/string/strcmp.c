#include <string.h>
#include <stddef.h>

int strcmp(const char* first, const char* second)
{
    size_t len1 = strlen(first);
    size_t len2 = strlen(second);

    if(len1 == len2)
    {
        return memcmp(first, second, len1);
    }
    return -1;
}