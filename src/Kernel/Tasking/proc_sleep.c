//
// Created by rmdir on 10/21/23.
//

#include "BalrogOS/Tasking/proc_sleep.h"
#include "BalrogOS/Tasking/process.h"
#include "klib/DataStructure/rbt.h"
#include "BalrogOS/CPU/PIT/pit.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Debug/debug_output.h"
#include <stddef.h>
#include <stdlib.h>

extern rbt_tree sleeper_tree;

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
    rbt_node* node = rbt_minimum(&sleeper_tree);
    if(node == NULL) {
        return;
    }

    sleeper_data* slpr = get_sleeper_data(node);
    if(!slpr) {
        rbt_delete(&sleeper_tree, node);
        return;
    }
    kernel_debug_output(KDB_LVL_ERROR, "wake up 0%p", slpr);

    process* process = slpr->process;
    if(!process) {
        rbt_delete(&sleeper_tree, node);
        return;
    }

    // sleeper might have been killed by an other process so check if it's still alive
    while(process->state & PROCESS_STATE_SLEEPING && pit_compare(&slpr->time)) {
        vmfree(slpr);
        // if the process is still sleeping, wake it up
        // else the process has been killed or is waiting
        // an other process
        if(process->state == PROCESS_STATE_SLEEPING) {
            proc_transfert_to_ready(process->pid, PROCESS_STATE_SLEEPING);
        } else if(process->state & PROCESS_STATE_SLEEPING) {
            process->state ^= PROCESS_STATE_SLEEPING;
        }
        process->sleeper_node = NULL;
        rbt_delete(&sleeper_tree, node);
        node = rbt_minimum(&sleeper_tree);
    }
}

size_t get_tree_key(timespec* time)
{
    return time->sec * 1000 + time->msec;
}

void sleep(timespec* time, process* proc)
{
    size_t key = get_tree_key(time);

    rbt_node* node = rbt_insert(&sleeper_tree, key);
    sleeper_data* slpr = vmalloc(sizeof(sleeper_data));
    proc->sleeper_node = node;
    slpr->process = proc;
    slpr->time = *time;
    node->value = slpr;
    proc_to_sleep(proc->pid, PROCESS_STATE_SLEEPING);
}

void remove_sleeper(process* proc)
{
    if(proc->sleeper_node == NULL) {
        return;
    }

    rbt_delete(&sleeper_tree, (rbt_node*) proc->sleeper_node);
    proc->sleeper_node = NULL;
}