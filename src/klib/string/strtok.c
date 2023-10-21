#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "BalrogOS/Debug/debug_output.h"

uintptr_t strspn(char * __restrict str, const char delim)
{
    size_t size = 0;

    while(str[size] == delim && str[size] != 0)
    {
        size++;
    }

    return size;
}

uintptr_t strcspn(char * __restrict str, const char delim)
{
    size_t size = 0;

    while(str[size] != delim && str[size] != 0)
    {
        size++;
    }

    return size;
}

char* strtok(char * __restrict str, const char delim)
{
    static char* current_ptr = 0;

    if(str == NULL)
    {
        str = current_ptr;
    }

    if(*str == 0)
    {
        current_ptr = str;
        return NULL;
    }

    str += strspn(str, delim);

    if(*str == 0)
    {
        current_ptr = str;
        return NULL;
    }

    char* end = str + strcspn(str, delim);

    if(*end == 0)
    {
        current_ptr = end;
        return str;
    }

    *end = 0;
    current_ptr = end + 1;
    return str;
}