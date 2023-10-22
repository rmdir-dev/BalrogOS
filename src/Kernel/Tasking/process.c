#include "BalrogOS/Tasking/process.h"
#include "klib/DataStructure/rbt.h"
#include <stdlib.h>
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/kheap.h"

rbt_tree process_tree;
rbt_tree sleeper_tree;
process_list rdy_proc_list = { NULL, 0, NULL};
extern process* current_running;

void init_process()
{
    rbt_init(&process_tree);
    rbt_init(&sleeper_tree);
}

void proc_insert_to_ready_queue(process* proc)
{
    rbt_node* new_node = rbt_insert(&process_tree, proc->pid);
    new_node->value = proc;

    if(rdy_proc_list.head == NULL)
    {
        if(rdy_proc_list.tail != NULL)
        {
            while(1){}
        }
        rdy_proc_list.head = proc;
        rdy_proc_list.tail = proc;
        proc->next = proc;
        proc->prev = NULL;
    } else
    {
        proc->next = rdy_proc_list.head;
        proc->prev = rdy_proc_list.tail;
        rdy_proc_list.tail->next = proc;
        rdy_proc_list.tail = proc;
    }

    rdy_proc_list.head->prev = rdy_proc_list.tail;
    rdy_proc_list.size++;

    proc->state = PROCESS_STATE_READY;
}

extern void schedule(size_t tick, uint16_t ms);

static int _proc_transfert_to_wait(process* proc)
{
    if(proc->state == PROCESS_STATE_READY || proc->state == PROCESS_STATE_RUNNING)
    {
        if(rdy_proc_list.size == 1)
        {
            //current_running = NULL;
            rdy_proc_list.head = NULL;
            rdy_proc_list.tail = NULL;
        } else if(proc == rdy_proc_list.head)
        {
            proc->next->prev = rdy_proc_list.tail;
            rdy_proc_list.tail->next = proc->next;
            rdy_proc_list.head = proc->next;
        } else if(proc == rdy_proc_list.tail)
        {
            proc->prev->next = rdy_proc_list.head;
            rdy_proc_list.head->prev = proc->prev;
            rdy_proc_list.tail = proc->prev;
        } else
        {
            proc->prev->next = proc->next;
            proc->next->prev = proc->prev;
        }
        rdy_proc_list.size--;

        return 0;
    }

    return -1;
}

int _proc_remove_process(process* proc)
{
    if(proc->state == PROCESS_STATE_DEAD || proc->state == PROCESS_STATE_ZOMBIE)
    {
        rbt_delete_key(&process_tree, proc->pid);

        return 0;
    }

    return -1;
}

void proc_wake_process(int* wating, uint8_t size)
{
    for(uint8_t i = 0; i < size; i++)
    {
        if(wating[i] != 0)
        {
            proc_transfert_to_ready(wating[i], PROCESS_STATE_WAITING);
        }
    }
}

/**
 * @brief Process MUST BE in waiting state to enter this function !
 *
 * @param proc
 */
static void _proc_kill(process* proc)
{
    proc->state = PROCESS_STATE_DEAD;

    if(proc->wait_size != 0)
    {
        proc_wake_process(&proc->waiting[0], proc->wait_size);
    }

    _proc_remove_process(proc);
    uintptr_t proc_addr = (uintptr_t)proc;

    // if proc is not a child then clean it.
    // or if the memory was copied then clean it.
    clean_process(proc, proc->child == 0 || !proc->forked_memory);


    if(proc == current_running)
    {
        if(proc == current_running->next)
        {
            current_running = NULL;
        }

        schedule(0, 0);
    }
}

void proc_kill_process(int pid)
{
    process* proc = proc_get_process(pid);

    if(_proc_transfert_to_wait(proc) == 0)
    {
        _proc_kill(proc);
    }
}

void proc_kill(process* proc, uint8_t force_schedule)
{
    if(
        proc->state & PROCESS_STATE_WAITING
        || proc->state & PROCESS_STATE_SLEEPING
        || proc->state == PROCESS_STATE_DEAD
        || proc->state == PROCESS_STATE_ZOMBIE
        || _proc_transfert_to_wait(proc) == 0
    )
    {
        _proc_kill(proc);
        if(force_schedule)
        {
            schedule(0, 0);
        }
    }
}

int proc_remove_process(int pid)
{
    process* proc = proc_get_process(pid);

    return _proc_remove_process(proc);
}

void proc_transfert_to_waiting(int pid)
{
    process* proc = proc_get_process(pid);

     if(_proc_transfert_to_wait(proc) == 0)
    {
        proc->state ^= PROCESS_STATE_READY | PROCESS_STATE_RUNNING;
         proc->state |= PROCESS_STATE_WAITING;
    }
}

void proc_to_sleep(int pid, uint8_t set_state)
{
    process* proc = proc_get_process(pid);

    if(_proc_transfert_to_wait(proc) == 0)
    {
        proc->state &= ~(PROCESS_STATE_READY) & ~(PROCESS_STATE_RUNNING);
        proc->state |= set_state;

        if(proc == current_running)
        {
            if(proc == current_running->next)
            {
                current_running = NULL;
            }

            schedule(0, 0);
        }
    }
}

int proc_add_to_waiting(int pid, int to_wait_pid)
{
    process* proc = proc_get_process(to_wait_pid);

    if(proc != 0 && proc->wait_size < 5)
    {
        proc->waiting[proc->wait_size] = pid;
        proc->wait_size++;
        return 0;
    }
    return -1;
}

void proc_transfert_to_ready(int pid, uint8_t expected_state)
{
    process* proc = proc_get_process(pid);

    if(!proc) {
        return;
    }

    if(proc->state == expected_state)
    {
        proc_insert_to_ready_queue(proc);
    }

    if(proc->state & expected_state) {
        proc->state ^= expected_state;
    }
}

process* proc_get_process(int pid)
{
    rbt_node* proc_node = rbt_search(&process_tree, pid);
    if(proc_node != 0)
    {
        return (process*) proc_node->value;
    }
    return NULL;
}