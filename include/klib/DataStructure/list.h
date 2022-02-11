#pragma once

#include "klib/Threading/kmutex.h"

typedef struct __list_node_t
{
    int key;
    void* value;
    struct __list_node_t* next;
} list_node_t;

typedef struct __list_t
{
    list_node_t* head;
    kmutex_t lock;
} list_t;

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
list_node_t* list_insert(list_t* list, int key);

/**
 * @brief 
 * 
 * @param list 
 * @param key 
 * @return list_node_t* 
 */
list_node_t* list_lookup(list_t* list, int key);