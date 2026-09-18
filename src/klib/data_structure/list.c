#include <stdlib.h>
#include "klib/data_structure/list.h"
#include "klib/threading/kmutex.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"

void list_init(list_t* list)
{
    // set the head to NULL so the end of the list is always equals to NULL.
    list->head = NULL;
    list->size = 0;
    kmutex_init(&list->lock);
}

list_node_t* list_insert(list_t* list, size_t key, void* value)
{
    list_node_t* node = vmalloc(sizeof(list_node_t));

    if(!node)
    {
        kernel_debug_output(KDB_LVL_ERROR, "Unable to create a new list link! 0%x", key);
        return NULL;
    }

    node->key = key;
    node->value = value;

    kmutex_lock(&list->lock);
    list->size++;
    node->next = list->head;
    list->head = node;
    kmutex_unlock(&list->lock);
    return node;
}

list_node_t* list_lookup(list_t* list, size_t key)
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
