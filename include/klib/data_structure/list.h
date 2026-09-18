#pragma once

#include "klib/threading/kmutex.h"

typedef struct __list_node_t
{
    size_t key;
    void* value;
    struct __list_node_t* next;
} list_node_t;

typedef struct __list_t
{
    list_node_t* head;
    kmutex_t lock;
    size_t size;
} list_t;

typedef int (compare_callback_t) (list_node_t* a, const void* b);

/**
 * @brief 
 * 
 * @param list 
 */
void list_init(list_t* list);

/**
 * @brief 
 * 
 * @param list 
 * @param key 
 * @return list_node_t* 
 */
list_node_t* list_insert(list_t* list, size_t key, void* value);

/**
 * @brief 
 * 
 * @param list 
 * @param key 
 * @return list_node_t* 
 */
list_node_t* list_lookup(list_t* list, size_t key);

/**
 *  @brief
 *
 * @param list
 * @param key
 * @return
 */
list_node_t* list_str_lookup(list_t* list, const char* key);

list_node_t* list_custom_lookup(list_t* list, const void* key, compare_callback_t compare);