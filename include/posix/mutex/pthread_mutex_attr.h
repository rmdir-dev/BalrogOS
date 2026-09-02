#pragma once

typedef struct _pthread_mutex_attr_t
{
    char _size[4];
    int _align;
} pthread_mutex_attr_t;