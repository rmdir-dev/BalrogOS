#pragma once

#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"

/**
 * @brief 
 * 
 * @param dev 
 * @param type 
 * @return int 
 */
uint32_t ext2_cache_search_bitmaps(fs_device* dev, uint8_t type);

/**
 * @brief 
 * 
 * @param dev 
 * @param inode_nbr 
 * @return ext2_idata* 
 */
ext2_idata* ext2_cache_search_inode(fs_device* dev, uint32_t inode_nbr);

/**
 * @brief remove an inode from the cache
 * 
 * @param inode_nbr 
 * @return int 
 */
int ext2_cache_delete_inode(uint32_t inode_nbr);

/**
 * @brief add a new file to the file system cache.
 * 
 * @param dev 
 * @param inode 
 * @return int 
 */
int ext2_add_file_to_cache(fs_device* dev, ext2_idata* inode, const char* filename);

/**
 * @brief 
 * 
 * @param inode 
 * @return int 
 */
int ext2_delete_file_from_cache(ext2_idata* inode);

/**
 * @brief Clear the ext2 cache
 * 
 * @return int 
 */
int ext2_clear_cache();