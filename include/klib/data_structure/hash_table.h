#pragma once

#include "klib/data_structure/list.h"

#define BUCKETS     100

typedef struct __hash_t
{
    list_t lists[BUCKETS];
} hash_t;

/**
 * @brief 
 * 
 * @param hash 
 */
void hash_init(hash_t* hash);

/**
 * @brief 
 * 
 * @param hash 
 * @param key 
 * @return list_node_t* 
 */
list_node_t* hash_insert(hash_t* hash, size_t key, void *value);

/**
 * @brief 
 * 
 * @param hash 
 * @param key 
 * @return list_node_t* 
 */
list_node_t* hash_lookup(hash_t* hash, size_t key);