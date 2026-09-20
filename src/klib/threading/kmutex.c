#include "klib/threading/kmutex.h"
#include "balrog/arch/x86_64/x86.h"
#include "balrog_os/tasking/process.h"
#include "balrog/thread/park.h"
#include "balrog_os/cpu/state/cpu_state.h"

int kmutex_init(kmutex_t* lock)
{
    lock->lock = 0;
    lock->flag = 0;
    lock->count = 0;
    mtx_queue_init(&lock->wait_queue);
    return 0;
}

int kmutex_lock(kmutex_t* lock)
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
    We first test with the solaris park unpark.
    The futex work a little differently :
        here the waiting queue in contain into the mutex
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
        process* current_running = get_current_process();
        mtx_queue_enqueue(&lock->wait_queue, current_running->pid);
        setpark();
        xchg(&lock->lock, 0);
        park();
    }

    return 0;
}

int kmutex_unlock(kmutex_t* lock)
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
    if(mtx_queue_empty(&lock->wait_queue))
    {
        lock->flag = 0;
    } else
    {
        /*
        unpark front the queue process
        wake up the process
        */
        unpark(mtx_queue_remove(&lock->wait_queue));
    }

    /* set the lock back to 0 atomically */
    xchg(&lock->lock, 0);

    return 0;
}