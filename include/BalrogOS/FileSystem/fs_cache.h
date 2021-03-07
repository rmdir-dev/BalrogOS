#pragma once

#include "filesystem.h"

/*
    FS dependant:
        - BLOCK BITMAP CACHE
        - INODE BITMAP CACHE
        - ROOT INODE
        - INODE CACHE -> RBT inode nbr = key (vmalloced)

    independant of the FS :
        - FILE CACHE (open file table)

    EXT2 dependant CACHE FUNCTIONS :
        - search_bitmap (search the fs bitmap) :
            - args :
                - device : 
                - type : 0 = block 1 = inode
            - return a free inode or block

        - search_inode :
            - args :
                - device
                - inode number
            - return an inode* to the requested inode

    FS INDEPENDANT functions :
        - add_file : 
            - add a file to the open_file_array
            - args :
                - pointer to the file data
                - char* file name
            - return the index of the file into the open file table
        - searh_file : 
            - return a file on the open_file_array

    file cache memory :
        - 4096 32MiB spaces (128GiB total)
        - array of 4096 bytes
            - flags :
                - 0x01  use/free
                - 0x02  
                - 0x04
                - 0x08
                - 0x10
                - 0x20
                - 0x40
                - 0x80
*/

/**
 * @brief 
 * 
 * @param buffer 
 * @param index 
 * @param filename 
 * @return int 
 */
int fs_cache_add_file(const char* filename, uint8_t* buffer, uint64_t size, uint32_t* index);

/**
 * @brief 
 * 
 * @param index 
 * @return uint8_t* 
 */
uint8_t* fs_cache_get_file(uint32_t index);

/**
 * @brief Return a new buffer into the file system memory
 *  
 * @param size the size of the buffer in byte
 * @return uint8_t* the buffer, return 0 if there are no memory left. 
 */
uint8_t* fs_cache_get_new_buffer(uint64_t size);

/**
 * @brief Free the buffer
 * 
 * @param index 
 * @return int 
 */
int fs_cache_close_file(uint32_t index);