#include "klib/data_structure/vector.h"
#include <string.h>
#include "balrog_os/memory/kheap.h"

static int __vector_grow(vector_t* vector)
{
    uint8_t* new_data = vmalloc(vector->__mem_size + (vector->__growth * vector->__data_size));

    if (!new_data)
    {
        return -1;
    }

    memcpy(new_data, vector->data, vector->__mem_size);
    vmfree(vector->data);
    vector->data = new_data;
    vector->__mem_size += vector->__growth * vector->__data_size;
    vector->__max_size += vector->__growth;

    return 0;
}

int vector_push(vector_t* vector, void* data)
{
    if (vector->current_size + 1 > vector->__max_size)
    {
        if(__vector_grow(vector) != 0)
        {
            return -1;
        }
    }

    memcpy(&vector->data[vector->current_size * vector->__data_size], data, vector->__data_size);
    vector->current_size++;

    return 0;
}

int vector_pop(vector_t* vector, size_t index, void* data)
{
    if (index >= vector->current_size)
    {
        return -1;
    }

    if (!data)
    {
        return -1;
    }

    memcpy(data, &vector->data[index * vector->__data_size], vector->__data_size);

    // if not last element of the list.
    if (index != vector->current_size - 1)
    {
        memmove(
            &vector->data[index * vector->__data_size],
            &vector->data[(index + 1) * vector->__data_size],
            (vector->current_size - index - 1) * vector->__data_size
        );
    }
    vector->current_size--;

    return 0;
}

void* vector_get(vector_t *vector, size_t index)
{
    if (index >= vector->current_size)
    {
        return NULL;
    }

    return &vector->data[index * vector->__data_size];
}

int vector_clear(vector_t* vector)
{
    if (!vector->data)
    {
        return -1;
    }

    vmfree(vector->data);
    vector->data = NULL;

    return 0;
}

int vector_init(vector_t* vector, size_t data_size, size_t default_growth)
{
    vector->__growth = default_growth;
    vector->__max_size = default_growth;
    vector->__data_size = data_size;
    vector->__mem_size = vector->__growth * data_size;
    vector->data = vmalloc(vector->__mem_size);
    vector->current_size = 0;

    return !vector->data ? -1 : 0;
}
