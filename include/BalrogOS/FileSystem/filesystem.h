#pragma once

#include <stdint.h>
#include "balrog/fs/fs_struct.h"
#include "klib/Threading/kmutex.h"

struct _file_system_t;

typedef enum _fs_format
{
    _EXT2_FORMAT = 0
}fs_format;

typedef struct _fs_file
{
    // full name of the file (real path)
    const char* name;
    // pointer to the datas.
    void* data;
    // file size
    uint64_t size;
    // read write
    uint8_t protection;
    // reference count.
    uint32_t reference;
    // inode number
    uint32_t inode_nbr;
} __attribute__((packed)) fs_file;

typedef struct _fs_fd
{
    // index in the file table
    uint32_t ftable_idx;
    // inode number
    uint32_t inode_nbr;
    // Current offset pointer
    uint8_t* offset;
} __attribute__((packed)) fs_fd;

typedef struct _fs_device_t
{
    char* name;
    kmutex_t lock;
    uint32_t unique_id;
    uint8_t type;
    void (*read)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    void (*write)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    struct _file_system_t* fs;
} __attribute__((packed)) fs_device_t;

typedef struct _file_system_t
{
    char* name;
    int (*probe)(fs_device_t* device);
    int (*open)(fs_device_t* dev, char* filename, fs_fd* fd);
    int (*close)(fs_device_t* dev, fs_fd* fd);
    int (*stat)(fs_device_t* dev, fs_fd* fd, fs_file_stat* stat);
    int (*read)(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd);
    int (*write)(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd);
    int (*touch)(fs_device_t* dev, char* filename);
    int (*list)(fs_device_t* dev, char* dirname, uint8_t* buffer);
    int (*mkdir)(fs_device_t* dev, char* dirname);
    void* fs_data;
} __attribute__((packed)) file_system_t;

void init_file_system();

/**
 * @brief 
 * 
 * @param name 
 * @param file 
 * @return int 
 */
int fs_get_file(const char* name, fs_file* file, fs_fd* fd);

/**
 * @brief 
 * 
 * @param name 
 * @param fd 
 * @return int 
 */
int fs_open(char* name, fs_fd* fd);

/**
 * @brief 
 * 
 * @param buffer 
 * @param len 
 * @param fd 
 * @return int 
 */
int fs_read(uint8_t* buffer, uint64_t len, fs_fd* fd);

/**
 * @brief 
 * 
 * @param fd 
 * @return int 
 */
int fs_close(fs_fd* fd);

/**
 * @brief 
 * 
 * @param fd 
 * @param stat 
 * @return int 
 */
int fs_fstat(fs_fd* fd, fs_file_stat* stat);