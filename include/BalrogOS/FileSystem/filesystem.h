#pragma once

#include <stdint.h>

struct _file_system;

typedef enum _fs_format
{
    _EXT2_FORMAT = 0
}fs_format;

typedef struct _fs_file
{
    // full name of the file (real path)
    char* name;
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

typedef struct _fs_dir_entry
{
    // inode nbr
    uint32_t inbr;
    // entry size
    uint16_t entry_size;
    // name length
    uint8_t name_len;
    // type (dir, regular file, link)
    uint8_t type;
    // file name
    char* name;
} fs_dir_entry;

typedef struct _fs_device
{
    char* name;
    uint32_t unique_id;
    uint8_t type;
    uint8_t (*read)(struct _fs_device* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    uint8_t (*write)(struct _fs_device* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    struct _file_system* fs;
} __attribute__((packed)) fs_device;

typedef struct _file_system
{
    char* name;
    int (*probe)(fs_device* device);
    int (*open)(fs_device* dev, const char* filename, fs_fd* fd);
    int (*close)(fs_device* dev, fs_fd* fd);
    int (*read)(fs_device* dev, uint8_t* buffer, uint64_t len, fs_fd* fd);
    int (*write)(fs_device* dev, uint8_t* buffer, uint64_t len, fs_fd* fd);
    int (*touch)(fs_device* dev, const char* filename);
    int (*list)(fs_device* dev, const char* dirname, uint8_t* buffer);
    int (*mkdir)(fs_device* dev, const char* dirname);
    void* fs_data;
} __attribute__((packed)) file_system;

void init_file_system();

/**
 * @brief 
 * 
 * @param name 
 * @param fd 
 * @return int 
 */
int fs_open(const char* name, fs_fd* fd);

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