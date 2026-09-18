#include "klib/data_structure/hash_table.h"

void hash_init(hash_t* hash)
{
    for(int i = 0; i < BUCKETS; i++)
    {
        list_init(&hash->lists[i]);
    }
}

list_node_t* hash_insert(hash_t* hash, int key, void *value)
{
    return list_insert(&hash->lists[key % BUCKETS], key, value);
}

list_node_t* hash_lookup(hash_t* hash, int key)
{
    return list_lookup(&hash->lists[key % BUCKETS], key);
}
