#pragma once
#include <stdint.h>
#include "queue_node.h"

/**
 * @brief the mutex queue is a non thread safe queue.
 * SHOULD ONLY be use by the kmutex and or FUTEX once implemented!
 * 
 */
typedef struct __mtx_queue_t
{
    queue_node_t* head;
    queue_node_t* tail;
} mtx_queue_t;

/**
 * @brief 
 * 
 * @param queue 
 */
void mtx_queue_init(mtx_queue_t* queue);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 */
void mtx_queue_enqueue(mtx_queue_t* queue, uint64_t value);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 * @return int 0 no error, -1 the queue is empty.
 */
int mtx_queue_dequeue(mtx_queue_t* queue, uint64_t* value);

/**
 * @brief remove the head of the queue, you MUST first check if the queue is not empty
 * 
 * @param queue 
 * @return int the value at the head of the queue.
 */
uint64_t mtx_queue_remove(mtx_queue_t* queue);

/**
 * @brief check if the queue is empty
 * 
 * @param queue 
 * @return int return 1 if empty
 */
int mtx_queue_empty(mtx_queue_t* queue);