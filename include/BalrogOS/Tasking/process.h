#pragma once
#include "tasking.h"

typedef struct _process_list
{
    process* head;
    unsigned size;
    process* tail;
} process_list;

/**
 * @brief push a new process into the ready queue
 * 
 * @param proc the process to push
 */
void proc_insert_to_ready_queue(process* proc);

/**
 * @brief Remove a non running process from the process tree
 * 
 * @param pid process ID
 * @return int 0 if no error, -1 if the process state wasn't allowed to be removed.
 */
int proc_remove_process(uint64_t pid);

/**
 * @brief transfert a process from the ready queue to the waiting
 * 
 * @param proc the process to transfert.
 */
void proc_transfert_to_waiting(uint64_t pid);

/**
 * @brief Transfert the process of said PID to the ready queue.
 * 
 * @param pid the process ID.
 */
void proc_transfert_to_ready(uint64_t pid);

/**
 * @brief Get the process object from it's PID
 * 
 * @param pid the process ID 
 * @return process* the process of the given ID
 */
process* proc_get_process(uint64_t pid);