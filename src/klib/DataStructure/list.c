#include <stdlib.h>
#include "klib/DataStructure/list.h"
#include "klib/Threading/kmutex.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Debug/debug_output.h"

void list_init(list_t* list)
{
    // set the head to NULL so the end of the list is always equals to NULL.
    list->head = NULL;
    kmutex_init(&list->lock);
}

list_node_t* list_insert(list_t* list, int key)
{
    list_node_t* node = vmalloc(sizeof(list_node_t));

    if(!node)
    {
        KERNEL_LOG_FAIL("Unable to create a new list link! 0%x", key);
        return NULL;
    }

    node->key = key;

    kmutex_lock(&list->lock);
    node->next = list->head;
    list->head = node;
    kmutex_unlock(&list->lock);
    return node;
}

list_node_t* list_lookup(list_t* list, int key)
{
    kmutex_lock(&list->lock);
    list_node_t* node = list->head;

    while (node)
    {
        if(node->key == key)
        {
            break;
        }
        node = node->next; 
    }
    
    kmutex_unlock(&list->lock);
    return node;
}
