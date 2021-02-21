#include "BalrogOS/Tasking/process.h"
#include "lib/DataStructure/rbt.h"
#include <stdlib.h>

rbt_tree process_tree = { NULL, NULL };
process_list rdy_proc_list = { NULL, 0, NULL};
extern process* current_running;

void proc_insert_to_ready_queue(process* proc)
{
    rbt_node* new_node = rbt_insert(&process_tree, proc->pid);
    new_node->value = proc;

    if(rdy_proc_list.head == NULL)
    {
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

int proc_remove_process(uint64_t pid)
{
    process* proc = proc_get_process(pid);

    if(proc->state == PROCESS_STATE_DEAD || proc->state == PROCESS_STATE_ZOMBIE)
    {
        rbt_delete_key(&process_tree, proc->pid);

        return 0;
    }

    return -1;
}

extern void schedule();

void proc_transfert_to_waiting(uint64_t pid)
{
    process* proc = proc_get_process(pid);

    if(proc->state == PROCESS_STATE_READY || proc->state == PROCESS_STATE_RUNNING)
    {
        if(rdy_proc_list.size == 1)
        {
            current_running = NULL;
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

        proc->state = PROCESS_STATE_WAITING;

        if(proc == current_running)
        {
            schedule();
        }
    }
}

void proc_transfert_to_ready(uint64_t pid)
{
    process* proc = proc_get_process(pid);
    
    if(proc->state == PROCESS_STATE_WAITING)
    {
        proc->next = rdy_proc_list.head;
        proc->prev = rdy_proc_list.tail;
        rdy_proc_list.tail->next = proc;
        rdy_proc_list.tail = proc;
        rdy_proc_list.size++;
        proc->state = PROCESS_STATE_READY;
    }
}

process* proc_get_process(uint64_t pid)
{
    rbt_node* proc_node = rbt_search(&process_tree, pid);
    return (process*) proc_node->value;
}