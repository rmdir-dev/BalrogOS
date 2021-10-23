#include "Mutex/pthread_mutex.h"
#include "Mutex/pthread_mutex_attr.h"
#include "balrog/thread/park.h"
#include "balrog/arch/x86_64/x86.h"

int pthread_mutex_init(pthread_mutex_t* lock, pthread_mutex_attr_t* attr)
{
    lock->lock = 0;
    lock->flag = 0;
    lock->count = 0;
    uqueue_init(&lock->wait_queue);
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
        This lock is only protecting the flag, it is then 
        released once the flag state is changed.
    */
    while(xchg(&lock->lock, 1))
    {}
    
    /*
    We first test with the solaris park unpark.
    The futex work a little differently :
        here the waiting queue is contain into the mutex
        the futex implement the waiting queue on the kernel side,
        probably inside an hashtable with the mutex address as index.
        Could also use the RBT but this does not need to be sorted,
        so the hash table will have faster insert.
    */
    if(lock->flag == 0)
    {
        lock->flag = 1;
        xchg(&lock->lock, 0);
    } else 
    {
        /*
        add process to parked list.
        */
        uqueue_enqueue(&lock->wait_queue, getpid());
        setpark();
        xchg(&lock->lock, 0);
        park();
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* lock)
{
    /*
        while test and set return 1 
        Test and set put 1 into the memory address of lock
        then return the old value.
        If the old value is equal to 0 then the lock is free
        else the lock is aquire by someone.
    */
    while(xchg(&lock->lock, 1))
    {}

    /*
    if the queue is empty then flag == 0 no one is parked
    */
    if(uqueue_empty(&lock->wait_queue))
    {
        lock->flag = 0;
    } else 
    {
        /*
        unpark front the queue process
        wake up the process
        */
        unpark(uqueue_remove(&lock->wait_queue));
    }

    /* set the lock back to 0 atomically */
    xchg(&lock->lock, 0);

    return 0;
}
