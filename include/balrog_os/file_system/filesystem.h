#pragma once

#include <stdint.h>
#include "balrog/fs/fs_struct.h"
#include "balrog_os/file_system/fs_devices.h"
#include "klib/data_structure/list.h"
#include "klib/threading/kmutex.h"

#define FS_DEVICE_TYPE_RAMFS    0
#define FS_DEVICE_TYPE_ATA      1
#define FS_DEVICE_TYPE_AHCI     2
#define FS_DEVICE_TYPE_XHCI     3

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

struct __vfs_node_t;

typedef struct _fs_fd
{
    // index in the file table
    uint32_t ftable_idx;
    // inode number
    uint32_t inode_nbr;
    // Current offset pointer
    uint8_t* offset;
    // linked device :
    fs_device_t* device;
    // linked vfs_node
    struct __vfs_node_t* vfs_node;
    // absolute path
    char* absolute_path;
} __attribute__((packed)) fs_fd;

#define fs_probe_callback(ROOT_TYPE_T) int (*probe)(ROOT_TYPE_T* device);
#define fs_open_callback(ROOT_TYPE_T) int (*open)(ROOT_TYPE_T* root, const char* filename, fs_fd* fd);
#define fs_close_callback(ROOT_TYPE_T) int (*close)(ROOT_TYPE_T* root, fs_fd* fd);
#define fs_stat_callback(ROOT_TYPE_T) int (*stat)(ROOT_TYPE_T* root, fs_fd* fd, fs_file_stat* stat);
#define fs_read_callback(ROOT_TYPE_T) int (*read)(ROOT_TYPE_T* root, uint8_t* buffer, uint64_t len, fs_fd* fd);
#define fs_write_callback(ROOT_TYPE_T) int (*write)(ROOT_TYPE_T* root, uint8_t* buffer, uint64_t len, fs_fd* fd);
#define fs_create_callback(ROOT_TYPE_T) int (*create)(ROOT_TYPE_T* root, const char* filename, uint64_t size);
#define fs_list_callback(ROOT_TYPE_T) int (*list)(ROOT_TYPE_T* root, const char* dirname, uint8_t* buffer);
#define fs_mkdir_callback(ROOT_TYPE_T) int (*mkdir)(ROOT_TYPE_T* root, const char* dirname);
#define fs_unlink_callback(ROOT_TYPE_T) int (*unlink)(ROOT_TYPE_T* root, const char* filename);
#define fs_rmdir_callback(ROOT_TYPE_T) int (*rmdir)(ROOT_TYPE_T* root, const char* dirname);

typedef struct _file_system_t
{
    char* name;
    fs_probe_callback(fs_device_t);
    fs_open_callback(fs_device_t);
    fs_close_callback(fs_device_t);
    fs_stat_callback(fs_device_t);
    fs_read_callback(fs_device_t);
    fs_write_callback(fs_device_t);
    fs_create_callback(fs_device_t);
    fs_list_callback(fs_device_t);
    fs_mkdir_callback(fs_device_t);
    fs_unlink_callback(fs_device_t);
    fs_rmdir_callback(fs_device_t);
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
 * @param mount_path
 * @param name_or_uuid
 * @return
 */
int fs_mount(const char* mount_path, const char* name_or_uuid);

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
 * @brief create an empty file, a create of size zero
 * 
 * @param filename 
 * @return int 
 */
int fs_touch(char* filename);

/**
 * @brief create a file of a given size
 * 
 * @param filename 
 * @param size the size the inode is created with, which is all a write can
 *             ever reach : __ext2_write refuses an offset past it
 * @return int 
 */
int fs_create(char* filename, uint64_t size);

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