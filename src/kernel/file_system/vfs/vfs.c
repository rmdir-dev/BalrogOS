#include "balrog_os/file_system/vfs/vfs.h"
#include "balrog_os/file_system/filesystem.h"
#include <string.h>
#include "balrog_os/memory/kheap.h"

#define vfs_guard(call) \
    { \
        kmutex_lock(&vfs_root->lock); \
        vfs_node_t* root = vfs_root->root; \
        int ret = 0; \
        call \
        kmutex_unlock(&vfs_root->lock); \
        return ret; \
    }

#define vfs_guard_sanitized(call) \
    { \
        kmutex_lock(&vfs_root->lock); \
        vfs_node_t* root = vfs_root->root; \
        size_t path_len = strlen(path); \
        char* sanitized_path = vmalloc(sizeof(char) * (path_len + 1)); \
        vfs_sanitize_path(path, sanitized_path, path_len); \
        size_t sanitized_path_len = strlen(sanitized_path); \
        vfs_node_t* node = __vfs_find_mount(root, sanitized_path); \
        if(!node) \
        { \
            vmfree(sanitized_path); \
            kmutex_unlock(&vfs_root->lock); \
            return -1; \
        } \
        int ret = 0; \
        call \
        vmfree(sanitized_path); \
        kmutex_unlock(&vfs_root->lock); \
        return ret; \
    }

static size_t __vfs_get_next_path_part_len(const char* path, size_t max_path_len)
{
    for (size_t i = 0; i < max_path_len; i++)
    {
        if (path[i] == '/')
        {
            return i;
        }
    }

    return max_path_len;
}

static vfs_node_t* __vfs_find_mount(vfs_node_t* root, const char* path)
{
    if (root->children_size == 0)
    {
        return root;
    }

    size_t path_len = strlen(path);
    size_t start = 0;

    if (path_len == 0)
    {
        return root;
    }

    if (path[start] == '/')
    {
        if (path_len == 1)
        {
            return root;
        }
        start++;
    }

    vfs_node_t* device_node = root;
    vfs_node_t* search_node = root;
    int searching = 1;

    while (searching)
    {
        size_t sub_path_len = __vfs_get_next_path_part_len(path + start, path_len - start);

        searching = 0;
        for (size_t i = 0; i < search_node->children_size; i++)
        {
            size_t name_len = strlen(search_node->children[i].name);
            if(name_len == sub_path_len && memcmp(path + start, search_node->children[i].name, sub_path_len) == 0)
            {
                searching = 1;
                if (search_node->children[i].device)
                {
                    device_node = &search_node->children[i];
                }

                search_node = &search_node->children[i];

                start += sub_path_len;
                if (path[start] == '/')
                {
                    start++;
                }
                break;
            }
        }

        if (search_node->children_size == 0)
        {
            break;
        }
    }

    return device_node;
}

static int __vfs_mount(vfs_root_t* vfs_root, const char* path, fs_device_t* device)
vfs_guard_sanitized({
    size_t depth = 0;
    size_t part_start = 0;
    size_t part_len = 0;

    while (part_start < sanitized_path_len)
    {
        if (sanitized_path[part_start] == '/')
        {
            part_start++;
        }

        part_len = __vfs_get_next_path_part_len(sanitized_path + part_start, sanitized_path_len - part_start);

        if (part_len == 0)
        {
            break;
        }

        depth++;

        if (depth > node->depth_from_root)
        {
            vfs_node_t* child_node = 0;
            for (size_t i = 0; i < node->children_size; i++)
            {
                size_t name_len = strlen(node->children[i].name);
                if (name_len == part_len && memcmp(sanitized_path + part_start, node->children[i].name, name_len) == 0)
                {
                    child_node = &node->children[i];
                    break;
                }
            }

            if (child_node)
            {
                node = child_node;
                part_start += part_len;
                continue;
            }

            if (node->children_buffer_size <= (node->children_size + 1))
            {
                vfs_node_t* new_children_ptr = vmalloc(sizeof(vfs_node_t) * (node->children_buffer_size + VFS_CHILDREN_GROWTH));
                for (size_t i = 0; i < node->children_size; i++)
                {
                    new_children_ptr[i].name = node->children[i].name;
                    new_children_ptr[i].device = node->children[i].device;
                    new_children_ptr[i].depth_from_root = node->children[i].depth_from_root;
                    new_children_ptr[i].parent = node->children[i].parent;
                    new_children_ptr[i].children_size = node->children[i].children_size;
                    new_children_ptr[i].children_buffer_size = node->children[i].children_buffer_size;
                    new_children_ptr[i].children = node->children[i].children;

                    for (size_t j = 0; j < new_children_ptr[i].children_size; j++)
                    {
                        new_children_ptr[i].children[j].parent = &new_children_ptr[i];
                    }
                }
                node->children_buffer_size = node->children_buffer_size + VFS_CHILDREN_GROWTH;
                vmfree(node->children);
                node->children = new_children_ptr;
            }
            vfs_node_t* new_node = &node->children[node->children_size];

            new_node->name = vmalloc(part_len + 1);
            memcpy(new_node->name, sanitized_path + part_start, part_len);
            new_node->name[part_len] = 0;
            new_node->depth_from_root = depth;
            new_node->parent = node;
            new_node->children_buffer_size = VFS_CHILDREN_GROWTH;
            new_node->children_size = 0;
            new_node->children = vmalloc(sizeof(vfs_node_t) * VFS_CHILDREN_GROWTH);
            new_node->device = 0;
            node->children_size++;

            node = new_node;
        }

        part_start += part_len;
    }

    node->device = device;
    device->path = vmalloc(path_len + 1);
    memcpy(device->path, path, path_len);
    device->path[path_len] = 0;
})

static int __vfs_umount(vfs_root_t* vfs_root, const char* path, fs_device_t* device)
vfs_guard_sanitized({
})

static int __vfs_open(vfs_root_t* vfs_root, const char* path, fs_fd* fd)
vfs_guard_sanitized({
    size_t shift = 0;

    for (size_t i = 0; i < node->depth_from_root; i++)
    {
        if (sanitized_path[shift] == '/')
        {
            shift++;
        }
        shift += __vfs_get_next_path_part_len(sanitized_path + shift, sanitized_path_len - shift);
    }

    if (node->depth_from_root > 0 && shift == sanitized_path_len)
    {
        shift = 0;
        sanitized_path[0] = '/';
        sanitized_path[1] = 0;
    }

    ret = node->device->fs->open(node->device, sanitized_path + shift, fd);
    fd->device = node->device;
})

static int __vfs_close(vfs_root_t* vfs_root, fs_fd* fd)
vfs_guard({
    ret = fd->device->fs->close(fd->device, fd);
})

static int __vfs_stat(vfs_root_t* vfs_root, fs_fd* fd, fs_file_stat* stat)
vfs_guard({
    ret = fd->device->fs->stat(fd->device, fd, stat);
})

static int __vfs_read(vfs_root_t* vfs_root, uint8_t* buffer, uint64_t len, fs_fd* fd)
vfs_guard({
    ret = fd->device->fs->read(fd->device, buffer, len, fd);
})

static int __vfs_write(vfs_root_t* vfs_root, uint8_t* buffer, uint64_t len, fs_fd* fd)
vfs_guard({
    ret = fd->device->fs->write(fd->device, buffer, len, fd);
})

static int __vfs_create(vfs_root_t* vfs_root, const char* path, uint64_t size)
vfs_guard_sanitized({
    ret = node->device->fs->create(node->device, sanitized_path, size);
})

static int __vfs_list(vfs_root_t* vfs_root, const char* path, uint8_t* buffer)
vfs_guard_sanitized({
    ret = node->device->fs->list(node->device, sanitized_path, buffer);
})

static int __vfs_mkdir(vfs_root_t* vfs_root, const char* path)
vfs_guard_sanitized({
    ret = node->device->fs->mkdir(node->device, sanitized_path);
})

static int __vfs_unlink(vfs_root_t* vfs_root, const char* path)
vfs_guard_sanitized({
    ret = node->device->fs->unlink(node->device, sanitized_path);
})

static int __vfs_rmdir(vfs_root_t* vfs_root, const char* path)
vfs_guard_sanitized({
    ret = node->device->fs->rmdir(node->device, sanitized_path);
})

int vfs_sanitize_path(const char* original_path, char* sanitized_path, size_t path_len)
{
    size_t part_start = 0;

    size_t part_len = 0;
    size_t copy_start = part_start;

    while (part_start < path_len)
    {
        if (original_path[part_start] == '/')
        {
            sanitized_path[copy_start] = '/';
            copy_start++;
        }

        while (original_path[part_start] == '/')
        {
            part_start++;
        }

        part_len = __vfs_get_next_path_part_len(original_path + part_start, path_len - part_start);
        memcpy(sanitized_path + copy_start, original_path + part_start, part_len);
        copy_start += part_len;
        part_start += part_len;
    }
    sanitized_path[copy_start] = 0;

    return 0;
}

int vfs_init(vfs_root_t* vfs_root, fs_device_t* device)
{
    vfs_node_t* root_node = vmalloc(sizeof(vfs_node_t));
    kmutex_init(&vfs_root->lock);
    kmutex_lock(&vfs_root->lock);

    root_node->device = device;
    root_node->name = "/";
    root_node->parent = NULL;
    root_node->depth_from_root = 0;
    root_node->children_size = 0;
    root_node->children = vmalloc(sizeof(vfs_node_t) * VFS_CHILDREN_GROWTH);
    root_node->children_buffer_size = VFS_CHILDREN_GROWTH;
    vfs_root->root = root_node;
    vfs_root->mount = __vfs_mount;
    vfs_root->umount = __vfs_umount;
    vfs_root->open = __vfs_open;
    vfs_root->close = __vfs_close;
    vfs_root->stat = __vfs_stat;
    vfs_root->read = __vfs_read;
    vfs_root->write = __vfs_write;
    vfs_root->create = __vfs_create;
    vfs_root->list = __vfs_list;
    vfs_root->mkdir = __vfs_mkdir;
    vfs_root->unlink = __vfs_unlink;
    vfs_root->rmdir = __vfs_rmdir;

    kmutex_unlock(&vfs_root->lock);

    return 0;
}
