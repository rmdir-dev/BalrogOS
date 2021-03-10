#include "BalrogOS/FileSystem/ext2/ext2_cache/ext2_cache.h"
#include "BalrogOS/FileSystem/fs_config.h"
#include "lib/DataStructure/rbt.h"
#include "BalrogOS/Memory/kheap.h"

struct _ext2_cache
{
    uint8_t* block_bitmap;
    uint8_t* inode_bitmap;
    rbt_tree inode_tree;
} ext2_cache;

uint32_t ext2_cache_search_bitmaps(fs_device* dev, uint8_t type)
{
    return 0;
}

extern ext2_inode ext2_get_inode(fs_device* dev, uint32_t inode_idx);

ext2_idata* ext2_cache_search_inode(fs_device* dev, uint32_t inode_nbr)
{
    rbt_node* node = rbt_search(&ext2_cache.inode_tree, inode_nbr);

    if(!node)
    {
        ext2_idata* file_data = vmalloc(sizeof(ext2_idata));

        if(!file_data)
        {
            return 0;
        }

        file_data->inode_nbr = inode_nbr;
        file_data->open = 0;
        file_data->file_id = 0;
        file_data->inode = ext2_get_inode(dev, inode_nbr);
        node = rbt_insert(&ext2_cache.inode_tree, inode_nbr);
        node->value = file_data;
    }

    return node->value;
}

int ext2_cache_delete_inode(uint32_t inode_nbr)
{
    rbt_delete(&ext2_cache.inode_tree, inode_nbr);
    return 0;
}

int ext2_add_file_to_cache(const char* filename, ext2_idata* inode, uint8_t* buffer)
{
    fs_cache_add_file(filename, buffer, inode->inode_nbr, inode->inode.size, &inode->file_id);
    return 0;
}

int ext2_close_file_from_cache(ext2_idata* inode, fs_fd* fd)
{
    if(fs_cache_close_file(fd->ftable_idx) == 0)
    {
        inode->open = 0;
    }
    return 0;
}

int ext2_clear_cache()
{
    rbt_clear_tree(&ext2_cache.inode_tree);
    return 0;
}
