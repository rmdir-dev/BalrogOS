#include "Mutex/pthread_mutex.h"
#include "Mutex/pthread_mutex_attr.h"
#include "plib/park.h"

int pthread_mutex_init(pthread_mutex_t* lock, pthread_mutex_attr_t* attr)
{
    lock->lock = 0;
    lock->flag = 0;
    lock->count = 0;
    unsafe_queue_init(&lock->wait_queue);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* lock)
{
    /* 
        while test and set return 1 
        Test and set put 1 into the memory address of lock
        then return the old value.
        If the old value is equal to 0 then the lock is free
        else the lock is aquire by someone.

    We first test with the solaris park unpark.
    The futex work a little differently :
        here the waiting queue in contain into the mutex
        the futex implement the waiting queue on the kernel side,
        probably inside an hashtable with the mutex address as index.

    */
    while(__atomic_test_and_set(&lock->lock, 0))
    {}
    
    if(lock->flag == 0)
    {
        lock->flag = 1;
        __atomic_clear(&lock->lock, 0);
    } else 
    {
        /*
        add process to parked list.
        */
        unsafe_queue_enqueue(&lock->wait_queue, getpid());
        setpark();
        __atomic_clear(&lock->lock, 0);
        park();
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* lock)
{
    /*
    */
    while(__atomic_test_and_set(&lock->lock, 0))
    {}

    /*
    if the queue is empty then flag == 0 no one is parked
    */
    if(unsafe_queue_empty(&lock->wait_queue))
    {
        lock->flag = 0;
    } else 
    {
        /*
        unpark front the queue process
        wake up the process
        */
        unpark(unsafe_queue_remove(&lock->wait_queue));
    }

    /* set the lock back to 0 atomically */
    __atomic_clear(&lock->lock, 0);

    return 0;
}
