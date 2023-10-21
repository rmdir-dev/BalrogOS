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

    while(node && pit_compare(&get_sleeper_data(node)->time)) {
        sleeper_data* slpr = get_sleeper_data(node);
        process* process = slpr->process;
        vmfree(slpr);
        // if the process is still sleeping, wake it up
        // else the process has been killed or is waiting
        // an other process
        if(process->state == PROCESS_STATE_SLEEPING) {
            proc_transfert_to_ready(process->pid, PROCESS_STATE_SLEEPING);
        }
        rbt_delete(&sleeper_tree, node);
        node = rbt_minimum(&sleeper_tree);
    }
}

size_t get_tree_key(timespec* time)
{
    return time->sec * 1000 + time->msec;
}

void sleep(timespec* time, process* process)
{
    size_t key = get_tree_key(time);

    rbt_node* node = rbt_insert(&sleeper_tree, key);
    sleeper_data* slpr = vmalloc(sizeof(sleeper_data));
    slpr->process = process;
    slpr->time = *time;
    node->value = slpr;
    proc_to_sleep(process->pid, PROCESS_STATE_SLEEPING);
}