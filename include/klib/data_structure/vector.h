#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct __vector_t
{
    size_t current_size;
    size_t __max_size;
    size_t __mem_size;
    size_t __data_size;
    size_t __growth;
    uint8_t* data;
} vector_t;

int vector_push(vector_t *vector, void *data);

int vector_pop(vector_t *vector, size_t index, void *data);

void* vector_get(vector_t *vector, size_t index);

int vector_clear(vector_t *vector);

int vector_init(vector_t *vector, size_t data_size, size_t default_growth);