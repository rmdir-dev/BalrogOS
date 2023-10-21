#include "toolkit/list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>


void list_init(list_t* list)
{
    // set the head to NULL so the end of the list is always equals to NULL.
    list->head = NULL;
    pthread_mutex_init(&list->lock, NULL);
}

list_node_t* list_insert(list_t* list, int key, list_node_t* node)
{
    if(!node) {
        node = malloc(sizeof(list_node_t));
    }

    if(!node)
    {
        printf("Unable to create a new list link! 0%x", key);
        return NULL;
    }

    node->key = key;

    pthread_mutex_lock(&list->lock);
    node->next = list->head;
    if(list->head) {
        list->head->prev = node;
    }
    list->head = node;
    node->prev = NULL;
    pthread_mutex_unlock(&list->lock);
    return node;
}

list_node_t* list_remove(list_t* list, int key) {
    pthread_mutex_lock(&list->lock);
    list_node_t* node = list->head;
    list_node_t* prev = NULL;

    while (node)
    {
        if(node->key == key)
        {
            if(prev)
            {
                prev->next = node->next;
                node->next->prev = prev;
            }
            else
            {
                list->head = node->next;
                node->next->prev = NULL;
            }
            break;
        }
        prev = node;
        node = node->next;
    }

    pthread_mutex_unlock(&list->lock);

    node->next = NULL;
    node->prev = NULL;
    return node;
}

list_node_t* list_remove_last(list_t* list)
{
    pthread_mutex_lock(&list->lock);
    list_node_t* node = list->head;
    list_node_t* prev = NULL;

    while (node)
    {
        if(!node->next)
        {
            if(prev)
            {
                prev->next = NULL;
            }
            else
            {
                list->head = NULL;
            }
            break;
        }
        prev = node;
        node = node->next;
    }

    pthread_mutex_unlock(&list->lock);
    return node;
}

list_node_t* list_lookup(list_t* list, int key)
{
    pthread_mutex_lock(&list->lock);
    list_node_t* node = list->head;

    while (node)
    {
        if(node->key == key)
        {
            break;
        }
        node = node->next;
    }

    pthread_mutex_unlock(&list->lock);
    return node;
}