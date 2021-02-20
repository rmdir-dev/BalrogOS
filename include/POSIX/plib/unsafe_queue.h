#pragma once
#include <stdint.h>

typedef struct __queue_node_t
{
    uint64_t value;
    struct __queue_node_t* next;
} queue_node_t;

/**
 * @brief the unsafe queue is a non thread safe queue.
 * 
 */
typedef struct __unsafe_queue_t
{
    queue_node_t* head;
    queue_node_t* tail;
} unsafe_queue_t;

/**
 * @brief 
 * 
 * @param queue 
 */
void unsafe_queue_init(unsafe_queue_t* queue);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 */
void unsafe_queue_enqueue(unsafe_queue_t* queue, uint64_t value);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 * @return int 0 no error, -1 the queue is empty.
 */
int unsafe_queue_dequeue(unsafe_queue_t* queue, uint64_t* value);

/**
 * @brief remove the head of the queue, you MUST first check if the queue is not empty
 * 
 * @param queue 
 * @return int the value at the head of the queue.
 */
uint64_t unsafe_queue_remove(unsafe_queue_t* queue);

/**
 * @brief check if the queue is empty
 * 
 * @param queue 
 * @return int return 1 if empty
 */
int unsafe_queue_empty(unsafe_queue_t* queue);