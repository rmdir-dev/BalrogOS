#include "balrog_os/file_system/vfs/vfs.h"
#include "balrog_os/file_system/filesystem.h"
#include <string.h>

#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/tasking/process.h"

typedef struct __vfs_insert_node_out_t
{
    vfs_node_t* last_search_node;
    vfs_node_t* last_device_node;
} vfs_find_node_out_t;

fs_device_t vfs_device;

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

static vfs_node_t* __vfs_find_mount(vfs_node_t* root, const char* path, vfs_find_node_out_t* out)
{
    out->last_search_node = root;
    out->last_device_node = root;

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

    int searching = 1;

    while (searching)
    {
        size_t sub_path_len = __vfs_get_next_path_part_len(path + start, path_len - start);

        searching = 0;
        for (size_t i = 0; i < out->last_search_node->children_size; i++)
        {
            size_t name_len = strlen(out->last_search_node->children[i].name);
            if(name_len == sub_path_len && memcmp(path + start, out->last_search_node->children[i].name, sub_path_len) == 0)
            {
                searching = 1;
                if (out->last_search_node->children[i].device)
                {
                    out->last_device_node = &out->last_search_node->children[i];
                }

                out->last_search_node = &out->last_search_node->children[i];

                start += sub_path_len;
                if (path[start] == '/')
                {
                    start++;
                }
                break;
            }
        }

        if (out->last_search_node->children_size == 0)
        {
            break;
        }
    }

    return out->last_device_node;
}

typedef struct __vfs_lookup_t
{
    char* sanitized_path;
    size_t sanitized_path_len;
    vfs_node_t* node;
    vfs_find_node_out_t vfs_find_node;
} vfs_lookup_t;

static inline void __vfs_guard_init(vfs_root_t* vfs_root)
{
    kmutex_lock(&vfs_root->lock);
}

static inline void __vfs_guard_release(vfs_root_t* vfs_root)
{
    kmutex_unlock(&vfs_root->lock);
}

static inline int __vfs_guard_sanitized_init(vfs_root_t* vfs_root, const char* path, vfs_lookup_t* out)
{
    kmutex_lock(&vfs_root->lock);

    vfs_node_t* root = vfs_root->root;
    size_t path_len = strlen(path);

    process* current_running = get_current_process();
    const char* cwd = (path[0] != '/' && current_running && current_running->cwd)
            ? current_running->cwd : 0;
    size_t cwd_len = cwd ? strlen(cwd) : 0;

    out->sanitized_path = vmalloc(cwd_len + 1 + path_len + 1);

    if(cwd)
    {
        memcpy(out->sanitized_path, cwd, cwd_len);
        out->sanitized_path[cwd_len] = '/';
        memcpy(out->sanitized_path + cwd_len + 1, path, path_len + 1);
        vfs_sanitize_path(out->sanitized_path, out->sanitized_path, cwd_len + 1 + path_len);
    }
    else
    {
        vfs_sanitize_path(path, out->sanitized_path, path_len);
    }

    out->sanitized_path_len = strlen(out->sanitized_path);
    out->node = __vfs_find_mount(root, out->sanitized_path, &out->vfs_find_node);

    if(!out->node)
    {
        vmfree(out->sanitized_path);
        kmutex_unlock(&vfs_root->lock);
        return -1;
    }

    return 0;
}

static inline void __vfs_guard_sanitize_release(vfs_root_t* vfs_root, vfs_lookup_t* lookup)
{
    vmfree(lookup->sanitized_path);
    kmutex_unlock(&vfs_root->lock);
}

static const char* __vfs_get_root_path(vfs_lookup_t* lookup)
{
    vfs_node_t* node = lookup->node;
    char* sanitized_path = lookup->sanitized_path;
    size_t sanitized_path_len = lookup->sanitized_path_len;
    int ret = 0;

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

    return sanitized_path + shift;
}

static vfs_node_t* __vfs_insert_nodes(vfs_lookup_t* lookup)
{
    vfs_node_t* node = lookup->node;
    char* sanitized_path = lookup->sanitized_path;
    size_t sanitized_path_len = lookup->sanitized_path_len;

    size_t depth = 0;
    size_t part_start = 0;
    size_t part_len = 0;
    int created = 0;

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
                if (child_node->type != VFS_NODE_TYPE_DIRECTORY)
                {
                    return 0;
                }

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
                    new_children_ptr[i].type = node->children[i].type;
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
            new_node->type = VFS_NODE_TYPE_DIRECTORY;
            memcpy(new_node->name, sanitized_path + part_start, part_len);
            new_node->name[part_len] = 0;
            new_node->depth_from_root = depth;
            new_node->parent = node;
            new_node->children_buffer_size = VFS_CHILDREN_GROWTH;
            new_node->children_size = 0;
            new_node->children = vmalloc(sizeof(vfs_node_t) * VFS_CHILDREN_GROWTH);
            new_node->device = 0;
            node->children_size++;
            created = 1;

            node = new_node;
        }

        part_start += part_len;
    }

    if (created)
    {
        node->type = 0;
    }

    return node;
}

static int __vfs_mount(vfs_root_t* vfs_root, const char* path, fs_device_t* device)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    size_t path_len = strlen(path);
    vfs_node_t* node = __vfs_insert_nodes(&lookup);

    if (!node)
    {
        __vfs_guard_sanitize_release(vfs_root, &lookup);
        return -1;
    }

    if (node->type != 0 && node->type != VFS_NODE_TYPE_DIRECTORY)
    {
        __vfs_guard_sanitize_release(vfs_root, &lookup);
        return -1;
    }

    node->type = VFS_NODE_TYPE_DIRECTORY;
    node->device = device;
    device->path = vmalloc(path_len + 1);
    memcpy(device->path, path, path_len);
    device->path[path_len] = 0;

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return 0;
}

static vfs_node_t* __vfs_add_vfs_node(vfs_root_t* vfs_root, const char* path, uint8_t type)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return 0;
    }

    if (lookup.sanitized_path_len > 0 && lookup.sanitized_path[lookup.sanitized_path_len - 1 ] == '/')
    {
        if (type != VFS_NODE_TYPE_DIRECTORY)
        {
            __vfs_guard_sanitize_release(vfs_root, &lookup);
            return 0;
        }

        lookup.sanitized_path_len--;
    }

    size_t depth = 1;
    // vfs should always receive absolute path
    for (size_t i = 1; i < lookup.sanitized_path_len; i++)
    {
        if (lookup.sanitized_path[i] == '/')
        {
            depth++;
        }
    }

    if (depth == lookup.vfs_find_node.last_search_node->depth_from_root)
    {
        vfs_node_t* ret = lookup.vfs_find_node.last_search_node;
        if (lookup.vfs_find_node.last_search_node->type != type)
        {
            kernel_debug_output(KDB_LVL_ERROR, "vfs : trying to assign a new type to an existing node");
            ret = 0;
        }

        __vfs_guard_sanitize_release(vfs_root, &lookup);
        return ret;
    }

    vfs_node_t* node = __vfs_insert_nodes(&lookup);

    if (!node)
    {
        __vfs_guard_sanitize_release(vfs_root, &lookup);
        return 0;
    }
    node->type = type;
    node->device = &vfs_device;

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return node;
}

static int __vfs_umount(vfs_root_t* vfs_root, const char* path, fs_device_t* device)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return 0;
}

static int __vfs_open(vfs_root_t* vfs_root, const char* path, fs_fd* fd)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    vfs_node_t* node = lookup.node;
    char* sanitized_path = lookup.sanitized_path;
    int ret = 0;

    fd->absolute_path = vmalloc(lookup.sanitized_path_len + 1);
    memcpy(fd->absolute_path, sanitized_path, lookup.sanitized_path_len);
    fd->absolute_path[lookup.sanitized_path_len] = 0;

    ret = node->device->fs->open(node->device, __vfs_get_root_path(&lookup), fd);
    fd->device = node->device;
    fd->vfs_node = node;

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

static int __vfs_close(vfs_root_t* vfs_root, fs_fd* fd)
{
    __vfs_guard_init(vfs_root);

    int ret = fd->device->fs->close(fd->device, fd);

    __vfs_guard_release(vfs_root);

    return ret;
}

static int __vfs_stat(vfs_root_t* vfs_root, fs_fd* fd, fs_file_stat* stat)
{
    __vfs_guard_init(vfs_root);

    int ret = fd->device->fs->stat(fd->device, fd, stat);

    __vfs_guard_release(vfs_root);

    return ret;
}

static int __vfs_read(vfs_root_t* vfs_root, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    __vfs_guard_init(vfs_root);

    int ret = fd->device->fs->read(fd->device, buffer, len, fd);

    __vfs_guard_release(vfs_root);

    return ret;
}

static int __vfs_write(vfs_root_t* vfs_root, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    __vfs_guard_init(vfs_root);

    int ret = fd->device->fs->write(fd->device, buffer, len, fd);

    __vfs_guard_release(vfs_root);

    return ret;
}

static int __vfs_create(vfs_root_t* vfs_root, const char* path, uint64_t size)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    int ret = lookup.node->device->fs->create(lookup.node->device, __vfs_get_root_path(&lookup), size);

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

static int __vfs_list(vfs_root_t* vfs_root, const char* path, uint8_t* buffer)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    int ret = lookup.node->device->fs->list(lookup.node->device, __vfs_get_root_path(&lookup), buffer);

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

static int __vfs_mkdir(vfs_root_t* vfs_root, const char* path)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    int ret = lookup.node->device->fs->mkdir(lookup.node->device, __vfs_get_root_path(&lookup));

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

static int __vfs_unlink(vfs_root_t* vfs_root, const char* path)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    int ret = lookup.node->device->fs->unlink(lookup.node->device, __vfs_get_root_path(&lookup));

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

static int __vfs_rmdir(vfs_root_t* vfs_root, const char* path)
{
    vfs_lookup_t lookup;

    if(__vfs_guard_sanitized_init(vfs_root, path, &lookup) != 0)
    {
        return -1;
    }

    int ret = lookup.node->device->fs->rmdir(lookup.node->device, __vfs_get_root_path(&lookup));

    __vfs_guard_sanitize_release(vfs_root, &lookup);

    return ret;
}

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

extern int __init_vfs_device(fs_device_t* vfs_device);

int vfs_init(vfs_root_t* vfs_root, fs_device_t* device)
{
    vfs_node_t* root_node = vmalloc(sizeof(vfs_node_t));
    kmutex_init(&vfs_root->lock);
    kmutex_lock(&vfs_root->lock);

    root_node->device = device;
    root_node->type = VFS_NODE_TYPE_DIRECTORY;
    root_node->name = "/";
    root_node->parent = NULL;
    root_node->depth_from_root = 0;
    root_node->children_size = 0;
    root_node->children = vmalloc(sizeof(vfs_node_t) * VFS_CHILDREN_GROWTH);
    root_node->children_buffer_size = VFS_CHILDREN_GROWTH;
    vfs_root->root = root_node;
    vfs_root->add_vfs = __vfs_add_vfs_node;
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
    __init_vfs_device(&vfs_device);

    kmutex_unlock(&vfs_root->lock);

    return 0;
}
