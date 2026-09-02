#include "klib/io/kprint.h"

int kputs(const char* str)
{
    kprint(str);
    return 1;
}