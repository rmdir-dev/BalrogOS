#include <string.h>
// TEMPORARY
#include "balrog_os/memory/kheap.h"

char* strdup(char* str)
{
    size_t len = strlen(str);
    char* ret = vmalloc(len + 1);

    if (!ret)
    {
        return 0;
    }

    memcpy(ret, str, len + 1);
    ret[len] = 0;

    return ret;
}