#pragma once
#include "lib/Threading/kmutex.h"
#include "lib/DataStructure/mtx_queue.h"
#include <stdint.h>

typedef struct __kmutex_t
{
    /* guard lock to protect the flag    */
    int lock;
    /* flag 1 = lock aquire | 0 = lock free    */
    uint8_t flag;
    /* counting waiting threads    */
    unsigned int count;
    /* a queue of currently waiting process */
    mtx_queue_t wait_queue;
} kmutex_t;

/**
 * @brief Initialize the mutex (mutual exclusion)
 * 
 * @param lock the lock to initialize
 * @return int 0 if no error, -1 if any error
 */
int kmutex_init(kmutex_t* lock);

/**
 * @brief lock the mutex and wait if it is already lock.
 * 
 * @param lock the mutex to lock
 * @return int 0 if no error -1 if errors occurred.
 */
int kmutex_lock(kmutex_t* lock);

/**
 * @brief release an owned lock
 * 
 * @param lock the lock to release
 * @return int 0 if no error, -1 if errors occurred.
 */
int kmutex_unlock(kmutex_t* lock);