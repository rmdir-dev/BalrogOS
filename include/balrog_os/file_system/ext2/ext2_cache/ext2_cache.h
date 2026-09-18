#pragma once

#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "klib/data_structure/rbt.h"

typedef struct _ext2_cache_t
{
    uint8_t* block_bitmap;
    uint8_t* inode_bitmap;
    rbt_tree inode_tree;
} ext2_cache_t;

/**
 * @brief 
 * 
 */
void ext2_cache_init(fs_device_t* dev);

/**
 * @brief 
 * 
 * @param dev 
 * @param type 
 * @return int 
 */
uint32_t ext2_cache_search_bitmaps(fs_device_t* dev, uint8_t type);

/**
 * @brief 
 * 
 * @param dev 
 * @param inode_nbr 
 * @return ext2_idata* 
 */
ext2_idata* ext2_cache_search_inode(fs_device_t* dev, uint32_t inode_nbr);

/**
 * @brief remove an inode from the cache
 * 
 * @param inode_nbr 
 * @return int 
 */
int ext2_cache_delete_inode(fs_device_t* dev, uint32_t inode_nbr);

/**
 * @brief add a new file to the file system cache.
 * 
 * @param dev 
 * @param inode 
 * @return int 
 */
int ext2_add_file_to_cache(const char* filename, ext2_idata* inode, uint8_t* buffer);

/**
 * @brief invalidate the cache
 *
 * @param filename
 * @param inode
 * @param buffer
 * @return
 */
int ext2_invalidate_cache(ext2_idata* inode);

/**
 * @brief 
 * 
 * @param inode 
 * @return int 
 */
int ext2_close_file_from_cache(ext2_idata* inode, fs_fd* fd);

/**
 * @brief Clear the ext2 cache
 * 
 * @return int 
 */
int ext2_clear_cache(fs_device_t* dev);