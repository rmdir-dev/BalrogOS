#include "Mutex/pthread_mutex.h"
#include "Mutex/pthread_mutex_attr.h"
#include <stdio.h>

int pthread_mutex_init(pthread_mutex_t* lock, pthread_mutex_attr_t* attr)
{
    lock->lock = 0;
    lock->flag = 0;
    lock->count = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* lock)
{
    /* currently a spin lock. 
       futex and or park/unpark syscalls */
    /* 
        while test and set return 1 
        Test and set put 1 into the memory address of lock
        then return the old value.
        If the old value is equal to 0 then the lock is free
        else the lock is aquire by someone.
    */
    while(__atomic_test_and_set(&lock->lock, 0))
    {
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* lock)
{
    /* set the lock back to 0 atomically */
    __atomic_clear(&lock->lock, 0);
    return 0;
}
