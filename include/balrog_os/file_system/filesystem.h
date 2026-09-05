#pragma once

#include <stdint.h>
#include "balrog/fs/fs_struct.h"
#include "klib/threading/kmutex.h"

#define FS_DEVICE_TYPE_RAMFS    0
#define FS_DEVICE_TYPE_ATA      1
#define FS_DEVICE_TYPE_AHCI     2

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
    void* drive;
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
    int (*unlink)(fs_device_t* dev, char* filename);
    int (*rmdir)(fs_device_t* dev, char* dirname);
    void* fs_data;
} __attribute__((packed)) file_system_t;

int init_file_system();

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
 * @param buffer 
 * @param len 
 * @param fd 
 * @return int 
 */
int fs_write(uint8_t* buffer, uint64_t len, fs_fd* fd);

/**
 * @brief create an empty file
 * 
 * @param filename 
 * @return int 
 */
int fs_touch(char* filename);

/**
 * @brief create an empty directory
 * 
 * @param dirname 
 * @return int 
 */
int fs_mkdir(char* dirname);

/**
 * @brief remove a file and free everything it holds
 * 
 * @param filename 
 * @return int 
 */
int fs_unlink(char* filename);

/**
 * @brief remove an empty directory
 * 
 * @param dirname 
 * @return int 
 */
int fs_rmdir(char* dirname);

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