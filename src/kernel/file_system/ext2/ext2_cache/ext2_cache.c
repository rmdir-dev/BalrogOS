#include "balrog_os/file_system/ext2/ext2_cache/ext2_cache.h"
#include "balrog_os/file_system/fs_config.h"
#include "balrog_os/file_system/fs_cache.h"
#include "klib/data_structure/rbt.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"

void ext2_cache_init(fs_device_t* dev)
{
    ext2_cache_t* cache = vmalloc(sizeof(ext2_cache_t));
    rbt_init(&cache->inode_tree);
    dev->fs->cache = cache;
}

uint32_t ext2_cache_search_bitmaps(fs_device_t* dev, uint8_t type)
{
    return 0;
}

extern ext2_inode ext2_get_inode(fs_device_t* dev, uint32_t inode_idx);

ext2_idata* ext2_cache_search_inode(fs_device_t* dev, uint32_t inode_nbr)
{
    ext2_cache_t* ext2_cache = dev->fs->cache;
    rbt_node* node = rbt_search(&ext2_cache->inode_tree, inode_nbr);

    if(!node)
    {
        ext2_idata* file_data = vmalloc(sizeof(ext2_idata));
        
        if(!file_data)
        {
            kernel_debug_output(KDB_LVL_CRITICAL, "error allocating inode data\n");
            return 0;
        }

        file_data->inode_nbr = inode_nbr;
        file_data->open = 0;
        file_data->file_id = 0;
        file_data->inode = ext2_get_inode(dev, inode_nbr);

        node = rbt_insert(&ext2_cache->inode_tree, inode_nbr);
        node->value = file_data;
    }
    
    return node->value;
}

int ext2_cache_delete_inode(fs_device_t* dev, uint32_t inode_nbr)
{
    ext2_cache_t* ext2_cache = dev->fs->cache;
    rbt_node* node = rbt_search(&ext2_cache->inode_tree, inode_nbr);
    if(node != 0)
    {
        vmfree(node->value);
        rbt_delete(&ext2_cache->inode_tree, node);
    }
    return 0;
}

int ext2_add_file_to_cache(const char* filename, ext2_idata* inode, uint8_t* buffer)
{
    inode->filename = filename;
    fs_cache_add_file(filename, buffer, inode->inode_nbr, inode->inode.size, &inode->file_id);
    return 0;
}

int ext2_invalidate_cache(ext2_idata* inode)
{
    if (inode->open)
    {
        int invalidated = fs_cache_invalidate(inode->file_id);
        inode->open = invalidated == 0 ? 0 : 1;
    }
    return 0;
}

int ext2_close_file_from_cache(ext2_idata* inode, fs_fd* fd)
{
    if(fs_cache_close_file(fd->ftable_idx) == 0)
    {
        kernel_debug_output(KDB_LVL_INFO, "closing file\n");
        inode->open = 0;
    }
    return 0;
}

int ext2_clear_cache(fs_device_t* dev)
{
    ext2_cache_t* ext2_cache = dev->fs->cache;
    rbt_clear_tree(&ext2_cache->inode_tree);
    return 0;
}
