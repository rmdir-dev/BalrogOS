//
// Created by rmdir on 10/21/23.
//

#include "balrog_os/tasking/proc_sleep.h"
#include "balrog_os/tasking/process.h"
#include "klib/data_structure/rbt.h"
#include "balrog_os/cpu/pit/pit.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"
#include <stddef.h>
#include <stdlib.h>

typedef struct sleeper_data_t {
    process* process;
    timespec time;
} sleeper_data;

sleeper_data* get_sleeper_data(rbt_node* node)
{
    return (sleeper_data*) node->value;
}

void wake_up(size_t tick, uint16_t ms)
{
    rbt_node* node = rbt_minimum(proc_get_sleeper_tree());

    while(node != NULL)
    {
        sleeper_data* slpr = get_sleeper_data(node);

        if(!slpr)
        {
            rbt_delete(proc_get_sleeper_tree(), node);
            node = rbt_minimum(proc_get_sleeper_tree());
            continue;
        }

        // if the first sleeper is still waiting,
        // then stop the waking up.
        if(!pit_compare(&slpr->time))
        {
            return;
        }

        process* process = slpr->process;

        // sleeper might have been killed by an other process so check if it's still alive
        if(process && process->state & PROCESS_STATE_SLEEPING)
        {
            if(process->state == PROCESS_STATE_SLEEPING)
            {
                proc_transfert_to_ready(process->pid, PROCESS_STATE_SLEEPING);
            } else
            {
                process->state ^= PROCESS_STATE_SLEEPING;
            }

            process->sleeper_node = NULL;
        }

        rbt_delete(proc_get_sleeper_tree(), node);
        vmfree(slpr);
        node = rbt_minimum(proc_get_sleeper_tree());
    }
}

size_t get_tree_key(timespec* time)
{
    return time->sec * 1000 + time->msec;
}

void sleep(timespec* time, process* proc)
{
    size_t key = get_tree_key(time);

    rbt_node* node = rbt_insert(proc_get_sleeper_tree(), key);
    sleeper_data* slpr = vmalloc(sizeof(sleeper_data));
    proc->sleeper_node = node;
    slpr->process = proc;
    slpr->time = *time;
    node->value = slpr;
    proc_to_sleep(proc->pid, PROCESS_STATE_SLEEPING);
}

void remove_sleeper(process* proc)
{
    if(proc->sleeper_node == NULL)
    {
        return;
    }

    rbt_delete(proc_get_sleeper_tree(), (rbt_node*) proc->sleeper_node);
    proc->sleeper_node = NULL;
}