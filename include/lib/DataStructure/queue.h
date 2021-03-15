#pragma once
#include <stdint.h>
#include <pthread.h>

typedef struct __queue_node_t
{
    uint64_t value;
    struct __queue_node_t* next;
} queue_node_t;

/**
 * @brief the unsafe queue is a non thread safe queue.
 * 
 */
typedef struct __queue_t
{
    queue_node_t* head;
    queue_node_t* tail;
    pthread_mutex_t head_lock, tail_lock;
} queue_t;

/**
 * @brief 
 * 
 * @param queue 
 */
void queue_init(queue_t* queue);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 */
void queue_enqueue(queue_t* queue, uint64_t value);

/**
 * @brief 
 * 
 * @param queue 
 * @param value 
 * @return int 0 no error, -1 the queue is empty.
 */
int queue_dequeue(queue_t* queue, uint64_t* value);

/**
 * @brief remove the head of the queue, you MUST first check if the queue is not empty
 * 
 * @param queue 
 * @return int the value at the head of the queue.
 */
uint64_t queue_remove(queue_t* queue);

/**
 * @brief check if the queue is empty
 * 
 * @param queue 
 * @return int return 1 if empty
 */
int queue_empty(queue_t* queue);