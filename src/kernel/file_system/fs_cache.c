#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/file_system/fs_config.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include <stddef.h>
#include "balrog_os/debug/debug_output.h"

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

    kernel_debug_output(KDB_LVL_ERROR, "fs cache : the %d slots are all taken, %s not cached",
            FS_MAX_FILE, filename);
    return FS_MAX_FILE;
}

int fs_cache_add_file(const char* filename, uint8_t* buffer, uint32_t inbr, uint64_t size, uint32_t* index)
{
    *index = _fs_cache_add_file_array(filename, inbr, buffer, size);

    // _fs_cache_add_file_array returns FS_MAX_FILE when the table is full.
    if(*index >= FS_MAX_FILE)
    {
        kernel_debug_output(KDB_LVL_ERROR, "fs cache : no slot for %s, inode %d", filename, inbr);
        return -1;
    }

    kernel_debug_output(KDB_LVL_VERBOSE, "fs cache : %s cached in slot %d, inode %d, %d bytes",
            filename, *index, inbr, size);

    return 0;
}

void fs_cache_increase_ref(uint32_t index)
{
    file_table[index].reference++;
}

fs_file* fs_cache_get_file(uint32_t index)
{
    if(index >= FS_MAX_FILE)
    {
        kernel_debug_output(KDB_LVL_ERROR, "fs cache : slot %d is not in the table", index);
        return 0;
    }

    return &file_table[index];
}

uint8_t* fs_cache_get_new_buffer(uint64_t size)
{
    uint64_t start_buffer_index = 0;
    uint64_t buffer_size = 0;
    uint8_t contiguous = 0;
    
    // Loop through the free pages buffer map
    // to find a contiguous free buffer that can hold the file.
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
        // get the index of the last block buffer
        uint64_t end_block_buffer = start_buffer_index + (buffer_size / FS_BUFFER_SIZE);
        void* addr = (void*)(FS_CACHE_OFFSET | (start_buffer_index * FS_BUFFER_SIZE));

        // set all blocks to 1 (taken)
        for(size_t i = start_buffer_index; i < end_block_buffer; i++)
        {
            free_buffer_map[i] = 1;
        }
        
        uint64_t to_map = size ? size : PAGE_SIZE;

        for(size_t i = 0; i < to_map; i += PAGE_SIZE)
        {
            void* p = pmm_calloc();

            if(!p || !vmm_set_page(0, addr + i, p, PAGE_PRESENT | PAGE_WRITE))
            {
                kernel_debug_output(KDB_LVL_ERROR, "fs cache : no page for 0%p, %d KiB buffer dropped",
                        addr + i, BYTE_TO_KiB(size));

                for(size_t done = 0; done < i; done += PAGE_SIZE)
                {
                    vmm_free_page(0, addr + done);
                }

                for(size_t b = start_buffer_index; b < end_block_buffer; b++)
                {
                    free_buffer_map[b] = 0;
                }

                return 0;
            }
        }

        return addr;
    }

    return 0;
}

static int __fs_cache_free_buffer(uint32_t index)
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

int fs_cache_invalidate(uint32_t index)
{
    if(index >= FS_MAX_FILE || file_table[index].reference == 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "fs cache : slot %d not invalidated, %s",
                index,
                index >= FS_MAX_FILE ? "index out of the table" : "nobody references it");
        return -1;
    }

    // if a file is still using this cache, then do not invalidate it
    // This would lead to a use after free.
    if (file_table[index].reference != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "fs cache : slot %d still has %d references, not invalidated",
                index, file_table[index].reference);
        return -1;
    }

    kernel_debug_output(KDB_LVL_VERBOSE, "fs cache : slot %d invalidated", index);
    __fs_cache_free_buffer(index);
    file_table[index].size = 0;
    return 0;
}

int fs_cache_close_file(uint32_t index)
{
    if(--file_table[index].reference == 0)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "fs cache : slot %d released, buffer freed", index);
        __fs_cache_free_buffer(index);
        file_table[index].size = 0;
        return 0;
    }

    kernel_debug_output(KDB_LVL_VERBOSE, "fs cache : slot %d still has %d references",
            index, file_table[index].reference);
    return -1;
}