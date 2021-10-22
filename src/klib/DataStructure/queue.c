#include "klib/DataStructure/queue.h"
#include "BalrogOS/Memory/kheap.h"

void queue_init(queue_t* queue)
{
    /*
    Dummy node.
    The dummy node will be the only node with a next value at NULL.
    */
    queue_node_t* tmp = kmalloc(sizeof(queue_node_t));
    tmp->next = NULL;
    queue->head = queue->tail = tmp;
    kmutex_init(&queue->head_lock);
    kmutex_init(&queue->tail_lock);
}

void queue_enqueue(queue_t* queue, uint64_t value)
{
    queue_node_t* node = kmalloc(sizeof(queue_node_t));
    
    node->value = value;
    node->next = NULL;

    kmutex_lock(&queue->tail_lock);
    queue->tail->next = node;
    queue->tail = node;
    kmutex_unlock(&queue->tail_lock);
}

int queue_dequeue(queue_t* queue, uint64_t* value)
{
    kmutex_lock(&queue->head_lock);
    queue_node_t* tmp = queue->head;
    queue_node_t* new_head = tmp->next;

    /* 
    if new_head is null then it is the dummy node.
    */
    if(!new_head)
    {
        kmutex_unlock(&queue->head_lock);
        return -1;
    }

    *value = new_head->value;
    queue->head = new_head;
    kmutex_unlock(&queue->head_lock);
    kfree(tmp);

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
    /* 
    Check if the next element is the dummy head.
    If it is then the queue is empty.
    */
    if(queue->head->next)
    {
        return 0;
    }
    return 1;
}