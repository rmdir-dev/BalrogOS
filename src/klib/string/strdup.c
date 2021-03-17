#include <string.h>
// TEMPORARY
#include "BalrogOS/Memory/kheap.h"

char* strdup(char* str)
{
    size_t len = strlen(str);
    char* ret = vmalloc(len + 1);
    memcpy(ret, str, len);
    ret[len] = 0;
    return ret;
}