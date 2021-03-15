#include "lib/DataStructure/queue.h"
#include "BalrogOS/Memory/kheap.h"

void queue_init(queue_t* queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

void queue_enqueue(queue_t* queue, uint64_t value)
{
    queue_node_t* node = kmalloc(sizeof(queue_node_t));
    
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

int queue_dequeue(queue_t* queue, uint64_t* value)
{
    kprint("dequeue 1\n");
    if(!queue->head)
    {
        return -1;
    }
    kprint("dequeue 2\n");
    queue_node_t* tmp = queue->head;
    *value = tmp->value;
    queue->head = tmp->next;
    kprint("dequeue 3\n");
    kfree(tmp);
    kprint("dequeue 4\n");
    return 0;
}

uint64_t queue_remove(queue_t* queue)
{
    uint64_t ret = 0;
    queue_dequeue(queue, &ret);
    return ret;
}

int queue_empty(queue_t* queue)
{
    if(queue->head)
    {
        return 0;
    }
    return 1;
}