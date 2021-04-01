#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/FileSystem/fs_config.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include <stddef.h>

static fs_file file_table[FS_MAX_FILE] = {};
static uint8_t free_buffer_map[4096] = {};

uint32_t _fs_cache_add_file_array(const char* filename, uint32_t inbr, uint8_t* buffer, uint64_t size)
{
    for(size_t i = 0; i < FS_MAX_FILE; i++)
    {
        if(file_table[i].size == 0)
        {
            file_table[i].name = filename;
            file_table[i].data = buffer;
            file_table[i].size = size;
            file_table[i].inode_nbr = inbr;
            return i;
        }
    }
    return 100;
}

int fs_cache_add_file(const char* filename, uint8_t* buffer, uint32_t inbr, uint64_t size, uint32_t* index)
{
    *index = _fs_cache_add_file_array(filename, inbr, buffer, size);

    if(*index > 100) 
    {
        return -1;
    }

    return 0;
}

void fs_cache_increase_ref(uint32_t index)
{
    file_table[index].reference++;
}

fs_file* fs_cache_get_file(uint32_t index)
{
    return &file_table[index];
}

uint8_t* fs_cache_get_new_buffer(uint64_t size)
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
        void* addr = (void*)(FS_CACHE_OFFSET | (start_buffer_index * FS_BUFFER_SIZE));

        for(size_t i = start_buffer_index; i < end_block_buffer; i++)
        {
            free_buffer_map[i] = 1;
        }
        
        for(size_t i = 0; i < size; i += PAGE_SIZE)
        {
            vmm_set_page(0, addr + i, pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
        }
        
        return addr;
    }

    return 0;
}

static int _fs_cache_free_buffer(uint32_t index)
{
    uint64_t buf_size = (file_table[index].size / FS_BUFFER_SIZE);
    uint64_t buf_idx = (((uintptr_t)file_table[index].data) & ~FS_CACHE_OFFSET);
    buf_idx = (buf_idx / FS_BUFFER_SIZE);

    for(size_t i = buf_idx; i < (buf_idx + buf_size); i++)
    {
        free_buffer_map[i] = 0;
    }

    for(size_t i = 0; i < file_table[index].size; i += PAGE_SIZE)
    {
        vmm_free_page(0, file_table[index].data + i);
    }

    return 0;
}

int fs_cache_close_file(uint32_t index)
{
    if(--file_table[index].reference == 0)
    {
        _fs_cache_free_buffer(index);
        file_table[index].size = 0;
        return 0;
    }
    return -1;
}