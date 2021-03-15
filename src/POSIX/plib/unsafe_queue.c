#include "plib/unsafe_queue.h"
#include "BalrogOS/Memory/kheap.h" // TEMPORARY!! switch to malloc once it is implemented!

void uqueue_init(uqueue_t* queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

void uqueue_enqueue(uqueue_t* queue, uint64_t value)
{
    //TODO Change to malloc
    uqueue_node_t* node = vmalloc(sizeof(uqueue_node_t));
    
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

int uqueue_dequeue(uqueue_t* queue, uint64_t* value)
{
    if(!queue->head)
    {
        return -1;
    }

    uqueue_node_t* tmp = queue->head;
    *value = tmp->value;
    queue->head = tmp->next;

    // TODO change to free()
    vmfree(tmp);

    return 0;
}

uint64_t uqueue_remove(uqueue_t* queue)
{
    uint64_t ret = 0;
    uqueue_dequeue(queue, &ret);
    return ret;
}

int uqueue_empty(uqueue_t* queue)
{
    if(queue->head)
    {
        return 0;
    }
    return 1;
}