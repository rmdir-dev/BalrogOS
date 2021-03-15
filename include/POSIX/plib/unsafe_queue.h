#pragma once
#include <stdint.h>

typedef struct __uqueue_node_t
{
    uint64_t value;
    struct __uqueue_node_t* next;
} uqueue_node_t;

/**
 * @brief the unsafe queue is a non thread safe queue.
 * 
 */
typedef struct __uqueue_t
{
    uqueue_node_t* head;
    uqueue_node_t* tail;
} uqueue_t;

/**
 * @brief 
 * 
 * @param queue 
 */
void uqueue_init(uqueue_t* queue);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 */
void uqueue_enqueue(uqueue_t* queue, uint64_t value);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 * @return int 0 no error, -1 the queue is empty.
 */
int uqueue_dequeue(uqueue_t* queue, uint64_t* value);

/**
 * @brief remove the head of the queue, you MUST first check if the queue is not empty
 * 
 * @param queue 
 * @return int the value at the head of the queue.
 */
uint64_t uqueue_remove(uqueue_t* queue);

/**
 * @brief check if the queue is empty
 * 
 * @param queue 
 * @return int return 1 if empty
 */
int uqueue_empty(uqueue_t* queue);