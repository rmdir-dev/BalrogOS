#pragma once
#include <stdint.h>

typedef struct __queue_node_t
{
    uint64_t value;
    struct __queue_node_t* next;
} queue_node_t;