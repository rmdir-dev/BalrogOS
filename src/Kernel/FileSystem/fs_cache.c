#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/FileSystem/fs_config.h"
#include <stddef.h>

fs_file open_file_array[FS_MAX_FILE] = {};
uint8_t free_buffer_map[4096] = {};

uint32_t _fs_cache_add_file_array(const char* filename, uint8_t* buffer, uint64_t size)
{
    for(size_t i = 0; i < FS_MAX_FILE; i++)
    {
        if(open_file_array[i].size == 0)
        {
            open_file_array[i].name = filename;
            open_file_array[i].data = buffer;
            open_file_array[i].size = size;
            return i;
        }
    }
    return 100;
}

int fs_cache_add_file(const char* filename, uint8_t* buffer, uint64_t size, uint32_t* index)
{
    *index = _fs_cache_add_file_array(filename, buffer, size);

    if(*index > 100) 
    {
        return -1;
    }

    return 0;
}

uint8_t* fs_cache_get_file(uint32_t index)
{
    return open_file_array[index].data;
}

uint8_t* fs_cache_new_get_buffer(uint64_t size)
{
    uint64_t start_buffer_index = 0;
    uint64_t buffer_size = 0;
    uint8_t contiguous = 0;

    for(size_t i = 0; i < 4096; i++)
    {
        if(free_buffer_map[i] == 0 && buffer_size == 0)
        {
            start_buffer_index = i;
            buffer_size = FS_BUFFER_SIZE;
            contiguous = 1;
        } else if(free_buffer_map[i] == 0 && contiguous == 1)
        {
            buffer_size += FS_BUFFER_SIZE;
        } else if(free_buffer_map[i] == 1)
        {
            contiguous = 0;
            start_buffer_index = 0;
            buffer_size = 0;
        }

        if(buffer_size >= size)
        {
            break;
        }
    }

    if(buffer_size >= size)
    {
        uint64_t end_block_buffer = start_buffer_index + (buffer_size / FS_BUFFER_SIZE);

        for(size_t i = start_buffer_index; i < end_block_buffer; i++)
        {
            free_buffer_map[i] = 1;
        }

        return (FS_CACHE_OFFSET | (start_buffer_index * FS_BUFFER_SIZE));
    }

    return 0;
}