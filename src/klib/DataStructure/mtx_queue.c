#include "klib/DataStructure/mtx_queue.h"
#include "BalrogOS/Memory/kheap.h" // TEMPORARY!! switch to malloc once it is implemented!

void mtx_queue_init(mtx_queue_t* queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

void mtx_queue_enqueue(mtx_queue_t* queue, uint64_t value)
{
    //TODO Change to malloc
    queue_node_t* node = vmalloc(sizeof(queue_node_t));
    
    node->value = value;
    node->next = NULL;

    if(!queue->head)
    {
        queue->head = node;
        queue->tail = node;
        return;
    }
    
    queue->tail->next = node;
    queue->tail = node;
}

int mtx_queue_dequeue(mtx_queue_t* queue, uint64_t* value)
{
    if(!queue->head)
    {
        return -1;
    }

    queue_node_t* tmp = queue->head;
    *value = tmp->value;
    queue->head = tmp->next;

    // TODO change to free()
    vmfree(tmp);

    return 0;
}

uint64_t mtx_queue_remove(mtx_queue_t* queue)
{
    uint64_t ret = 0;
    mtx_queue_dequeue(queue, &ret);
    return ret;
}

int mtx_queue_empty(mtx_queue_t* queue)
{
    if(queue->head)
    {
        return 0;
    }
    return 1;
}