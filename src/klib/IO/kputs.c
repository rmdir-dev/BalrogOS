#include "klib/IO/kprint.h"

int kputs(const char* str)
{
    kprint(str);
    return 1;
}