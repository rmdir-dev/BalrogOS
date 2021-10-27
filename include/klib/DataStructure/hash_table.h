#pragma once

#include "klib/DataStructure/list.h"

#define BUCKETS     100

typedef struct __hash_t
{
    list_t lists[BUCKETS];
} hash_t;

void hash_init(hash_t* hash);
list_node_t* hash_insert(hash_t* hash, int key);
list_node_t* hash_lookup(hash_t* hash, int key);