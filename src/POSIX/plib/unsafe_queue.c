#include "plib/unsafe_queue.h"
#include "BalrogOS/Memory/kheap.h" // TEMPORARY!! switch to malloc once it is implemented!

void unsafe_queue_init(unsafe_queue_t* queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

void unsafe_queue_enqueue(unsafe_queue_t* queue, uint64_t value)
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

int unsafe_queue_dequeue(unsafe_queue_t* queue, uint64_t* value)
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

uint64_t unsafe_queue_remove(unsafe_queue_t* queue)
{
    uint64_t ret = 0;
    unsafe_queue_dequeue(queue, &ret);
    return ret;
}

int unsafe_queue_empty(unsafe_queue_t* queue)
{
    if(queue->head)
    {
        return 0;
    }
    return 1;
}