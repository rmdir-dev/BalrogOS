#include <string.h>

// TODO test with long words -> 4 bytes by 4 bytes https://github.com/lattera/glibc/blob/master/string/strlen.c
size_t strlen(const char* str)
{
    size_t length = 0;

    while(str[length])
    {
        length++;
    }

    return length;
}