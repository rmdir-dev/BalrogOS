#pragma once

#include "balrog_os/file_system/filesystem.h"

// Virtual File System (representation of mount points).

#define VFS_CHILDREN_GROWTH         5

#define VFS_NODE_TYPE_DIRECTORY    1
#define VFS_NODE_TYPE_FILE         2

typedef struct __vfs_node_t
{
    char* name;
    uint8_t type;
    fs_device_t* device;
    size_t depth_from_root;
    struct __vfs_node_t* parent;
    size_t children_size;
    size_t children_buffer_size;
    struct __vfs_node_t* children;
} vfs_node_t;

#define fs_add_vfs_callback(ROOT_TYPE_T) vfs_node_t* (*add_vfs)(ROOT_TYPE_T* root, const char* mountpoint, uint8_t type);
#define fs_mount_callback(ROOT_TYPE_T) int (*mount)(ROOT_TYPE_T* root, const char* mountpoint, fs_device_t* device);
#define fs_umount_callback(ROOT_TYPE_T) int (*umount)(ROOT_TYPE_T* root, const char* mountpoint, fs_device_t* device);

typedef struct __vfs_root_t
{
    vfs_node_t* root;
    kmutex_t lock;
    fs_add_vfs_callback(struct __vfs_root_t);
    fs_mount_callback(struct __vfs_root_t);
    fs_umount_callback(struct __vfs_root_t);
    fs_open_callback(struct __vfs_root_t);
    fs_close_callback(struct __vfs_root_t);
    fs_stat_callback(struct __vfs_root_t);
    fs_read_callback(struct __vfs_root_t);
    fs_write_callback(struct __vfs_root_t);
    fs_create_callback(struct __vfs_root_t);
    fs_list_callback(struct __vfs_root_t);
    fs_mkdir_callback(struct __vfs_root_t);
    fs_unlink_callback(struct __vfs_root_t);
    fs_rmdir_callback(struct __vfs_root_t);
} vfs_root_t;

typedef struct __vfs_device_t
{

} vfs_device_t;

/**
 * @brief sanitize path //var//log become /var/log
 *
 * @param original_path
 * @param sanitized_path
 * @param path_len
 * @return
 */
int vfs_sanitize_path(const char* original_path, char* sanitized_path, size_t path_len);

/**
 *
 * @return
 */
int vfs_init(vfs_root_t* root, fs_device_t* device);