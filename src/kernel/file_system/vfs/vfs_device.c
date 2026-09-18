#include "balrog_os/file_system/vfs/vfs.h"
#include "balrog_os/file_system/filesystem.h"
#include <string.h>

#include "balrog_os/debug/debug_output.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/memory/kheap.h"

file_system_t vfs_file_system;

int __vfs_device_open(fs_device_t* device, const char* filename, fs_fd* fd)
{
    return 0;
}

int __vfs_device_close(fs_device_t* device, fs_fd* fd)
{
    return 0;
}

int __vfs_device_stat(fs_device_t* device, fs_fd* fd, fs_file_stat* stat)
{
    if (!fd->vfs_node || fd->vfs_node->type == VFS_NODE_TYPE_FILE)
    {
        // TODO stat file
        return -1;
    }

    stat->size = 0;
    stat->size = EXT2_DIR_ENTRY_SIZE(1); // .
    stat->size = EXT2_DIR_ENTRY_SIZE(2); // ..
    for (size_t i = 0; i < fd->vfs_node->children_size; i++)
    {
        // already + 1 as there is a char at the end of fs_dir_entry
        stat->size += EXT2_DIR_ENTRY_SIZE(strlen(fd->vfs_node->children[i].name));
    }
    stat->size += sizeof(fs_dir_entry);

    return 0;
}

void __register_entry(fs_dir_entry* entry, enum ext2_dir_entry_type type, const char* name)
{
    entry->inbr = -1;
    entry->name_len = strlen(name);
    entry->entry_size = EXT2_DIR_ENTRY_SIZE(entry->name_len);
    entry->type = type;
    memcpy(&entry->name, name, entry->name_len);
}

int __vfs_device_read(fs_device_t* device, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    if (!fd->vfs_node || fd->vfs_node->type == VFS_NODE_TYPE_FILE)
    {
        // TODO read file
        return -1;
    }

    fs_dir_entry* entry = buffer;
    __register_entry(entry, EXT2_TYPE_DIRECTORY, ".");
    buffer += entry->entry_size;
    entry = buffer;

    __register_entry(entry, EXT2_TYPE_DIRECTORY, "..");
    buffer += entry->entry_size;
    entry = buffer;

    for (size_t i = 0; i < fd->vfs_node->children_size; i++)
    {
        vfs_node_t* child = &fd->vfs_node->children[i];
        __register_entry(entry, EXT2_TYPE_BLOCK_DEVICE, child->name);
        buffer += entry->entry_size;
        entry = buffer;
    }
    // null terminated list.
    entry->inbr = 0;
    return 0;
}

int __vfs_device_write(fs_device_t* device, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    return -1;
}

int __vfs_device_create(fs_device_t* device, const char* filename, uint64_t size)
{
    return -1;
}

int __vfs_device_list(fs_device_t* device, const char* dirname, uint8_t* buffer)
{
    return -1;
}

int __vfs_device_mkdir(fs_device_t* device, const char* dirname)
{
    return -1;
}

int __vfs_device_unlink(fs_device_t* device, const char* filename)
{
    return -1;
}

int __vfs_device_rmdir(fs_device_t* device, const char* dirname)
{
    return -1;
}


int __init_vfs_device(fs_device_t* vfs_device)
{
    vfs_device->fs = &vfs_file_system;
    vfs_file_system.open = __vfs_device_open;
    vfs_file_system.close = __vfs_device_close;
    vfs_file_system.stat = __vfs_device_stat;
    vfs_file_system.read = __vfs_device_read;
    vfs_file_system.write = __vfs_device_write;
    vfs_file_system.create = __vfs_device_create;
    vfs_file_system.list = __vfs_device_list;
    vfs_file_system.mkdir = __vfs_device_mkdir;
    vfs_file_system.unlink = __vfs_device_unlink;
    vfs_file_system.rmdir = __vfs_device_rmdir;

    return 0;
}