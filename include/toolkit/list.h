//
// Created by rmdir on 10/21/23.
//

#ifndef BALROGOS_LIST_H
#define BALROGOS_LIST_H

#include <stdint.h>
#include <pthread.h>

typedef struct __list_node_t
{
    int key;
    void* value;
    struct __list_node_t* next;
    struct __list_node_t* prev;
} list_node_t;

typedef struct __list_t
{
    list_node_t* head;
    list_node_t* tail;
    pthread_mutex_t lock;
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
list_node_t* list_insert(list_t* list, int key, list_node_t* node);

/**
 * @brief
 *
 * @param list
 * @param key
 * @return list_node_t*
 */
list_node_t* list_remove(list_t* list, int key);

/**
 * @brief
 *
 * @param list
 * @return list_node_t*
 */
list_node_t* list_remove_last(list_t* list);

/**
 * @brief
 *
 * @param list
 * @param key
 * @return list_node_t*
 */
list_node_t* list_lookup(list_t* list, int key);

#endif //BALROGOS_LIST_H
