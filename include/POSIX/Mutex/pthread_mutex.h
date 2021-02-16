#pragma once
#include "pthread_mutex_attr.h"
#include <stdint.h>

typedef struct __pthread_mutex_t
{
    /* guard lock to protect the flag    */
    int lock;
    /* flag 1 = lock aquire | 0 = lock free    */
    uint8_t flag;
    /* counting waiting threads    */
    unsigned int count;
} pthread_mutex_t;

/**
 * @brief Initialize the mutex (mutual exclusion)
 * 
 * @param lock the lock to initialize
 * @param attr 
 * @return int 0 if no error, -1 if any error
 */
int pthread_mutex_init(pthread_mutex_t* lock, pthread_mutex_attr_t* attr);

/**
 * @brief lock the mutex and wait if it is already lock.
 * 
 * @param lock the mutex to lock
 * @return int 0 if no error -1 if errors occurred.
 */
int pthread_mutex_lock(pthread_mutex_t* lock);

/**
 * @brief release an owned lock
 * 
 * @param lock the lock to release
 * @return int 0 if no error, -1 if errors occurred.
 */
int pthread_mutex_unlock(pthread_mutex_t* lock);