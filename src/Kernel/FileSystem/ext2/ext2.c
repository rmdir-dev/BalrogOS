#include "BalrogOS/FileSystem/ext2/ext2_cache/ext2_cache.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"
#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Drivers/Disk/ata/ata.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/pmm.h"
#include "klib/IO/kprint.h"
#include "ext2_config.h"
#include "klib/ktime.h"

#include <unistd.h>
#include <string.h>

/*
    UTILITIES
*/

typedef struct _entry_read_dir_entries
{
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;
} entry_read_dir_entries;

static char** _ext2_get_path(char* src, const char delimiter, size_t* out_size, uint8_t* from_root)
{
    *from_root = 0;
    if(src[0] == '/')
    {
        src++;
        *from_root = 1;
    }

    char** ret = 0;
    size_t count = 0;
    char* tmp = src;
    char* last_delimiter = 0;
    char delim[2] = { delimiter, 0 };
    *out_size = 0;

    while(*tmp)
    {
        if(delimiter == *tmp)
        {
            count++;
            last_delimiter = tmp;
        }
        tmp++;
    }

    count += last_delimiter < (src + strlen(src) - 1);

    count++;

    ret = vmalloc(sizeof(char*) * count);

    if(ret)
    {
        size_t idx = 0;
        char* token = strtok(src, '/');

        while(token)
        {
            if(idx < count)
            {
                *(ret + idx++) = token;
                token = strtok(NULL, '/');
            }
        }

        if(idx == count - 1)
        {
            *(ret + idx) = 0;
            *out_size = idx;
        }
        
    }

    return ret;
}

static inline uint32_t _ext2_find_free_bitmap(fs_device_t* dev, size_t bitmap_size, size_t start_idx, uint32_t sec_per_block, size_t map_idx_start)
{
    size_t allocated = 0;

    uint8_t block_bitmap[512];

    for(size_t map_idx = map_idx_start; map_idx < bitmap_size; map_idx += 512)
    {

        dev->read(dev, &block_bitmap[0], (start_idx * sec_per_block) + (map_idx / 512), 1);

        for(size_t i = 0; i < 512; i++)
        {
            if(block_bitmap[i] != 0xff)
            {
                for(size_t bit = 0; bit < 8; bit++)
                {
                    if(~block_bitmap[i] & (1 << bit))
                    {
                        /* update the bitmap */
                        block_bitmap[i] |= (1 << bit);
                        dev->write(dev, &block_bitmap[0], (start_idx * sec_per_block) + (map_idx / 512), 1);

                        return (bit + (i * 8) + (4096 * (map_idx / 512))) + 1;
                    }
                }
            }
        }
    }
    return -1;
}

static int _ext2_update_sb_and_blk_desc(fs_device_t* dev, ext2_fs_data* fs_data)
{
    dev->write(dev, (void*)&fs_data->sb, 2, 2);
    if(fs_data->block_size == 1024)
    {
        dev->write(dev, (void*)&fs_data->blk_grp_desc,  1 * fs_data->sec_per_block, 1);
    } else 
    {
        dev->write(dev, (void*)&fs_data->blk_grp_desc,  2 * fs_data->sec_per_block, 1);
    }
    return 0;
}

/*
    BLOCKS
*/

static int _ext2_free_alloc_block(fs_device_t* dev, uint32_t block_id)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;
    size_t block_bitmap_size = fs_data->sb.blocks / 8;
    uint8_t block_bitmap[512];
    /*
    952 
    952/8
    119/512 = 0 = map sector idx
    952 % 8 = bit 
    */
    dev->read(dev, (void*)&block_bitmap, ((block_id / 8) / 512), 1);
    block_bitmap[block_id / 8] &= (0xff ^ (0 << (block_id % 8)));
    dev->write(dev, (void*)&block_bitmap, ((block_id / 8) / 512), 1);
    return 0;
}

static uint32_t _ext2_find_higher_half_free_blocks(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    /* TODO update superblock */
    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = _ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap, 
            fs_data->sec_per_block, block_bitmap_size / 2);
    if(block_id)
    {
        fs_data->sb.unalloc_blocks--;
        fs_data->blk_grp_desc.num_of_unalloc_block--;
        _ext2_update_sb_and_blk_desc(dev, fs_data);
    }
    return block_id;
}

static uint32_t _ext2_find_free_blocks(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    /* TODO update superblock */
    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = _ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap, fs_data->sec_per_block, 0);
    if(block_id)
    {
        fs_data->sb.unalloc_blocks--;
        fs_data->blk_grp_desc.num_of_unalloc_block--;
        _ext2_update_sb_and_blk_desc(dev, fs_data);
    }
    return block_id;
}

/*
    INODES
*/

static uint32_t _ext2_find_free_inode(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = _ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_inode_usage_bitmap, fs_data->sec_per_block, 0);
    if(block_id)
    {
        fs_data->sb.unalloc_inodes--;
        fs_data->blk_grp_desc.num_of_unalloc_inode--;
        _ext2_update_sb_and_blk_desc(dev, fs_data);
    }
    return block_id;
}

static int _ext2_update_inode_table(fs_device_t* dev, uint32_t inode_idx, ext2_inode* inode)
{
    uint32_t tbl_str_blc_addr = (inode_idx - 1) / 32;
    ext2_fs_data* fs_data = dev->fs->fs_data;
    ext2_inode* inode_table = (void*)P2V(pmm_calloc());
    dev->read(dev, (void*)inode_table, (fs_data->blk_grp_desc.block_addr_of_inode_table + tbl_str_blc_addr) * fs_data->sec_per_block, 8);
    inode_table[(inode_idx - 1) % 32] = *inode;
    dev->write(dev, (void*)inode_table, (fs_data->blk_grp_desc.block_addr_of_inode_table + tbl_str_blc_addr) * fs_data->sec_per_block, 8);
    pmm_free((void*)V2P(inode_table));
    return 0;
}

ext2_inode ext2_get_inode(fs_device_t* dev, uint32_t inode_idx)
{
    uint32_t tbl_str_blc_addr = (inode_idx - 1) / 32;
    ext2_fs_data* fs_data = dev->fs->fs_data;
    ext2_inode* inode_table = (void*)P2V(pmm_calloc());
    dev->read(dev, (void*)inode_table, (fs_data->blk_grp_desc.block_addr_of_inode_table + tbl_str_blc_addr) * fs_data->sec_per_block, 8);

    ext2_inode ret = inode_table[(inode_idx - 1) % 32];
    pmm_free((void*)V2P(inode_table));

    return ret;
}

static uint32_t _ext2_create_new_inode(fs_device_t* dev, uint32_t file_size, uint16_t mode)
{
    uint32_t new_id_inode = _ext2_find_free_inode(dev);
    
    ext2_inode new_inode = ext2_get_inode(dev, new_id_inode);
    new_inode.mode = mode;
    new_inode.user_id = 0;
    new_inode.size = file_size;
    new_inode.access_time = ktime(NULL);
    new_inode.create_time = ktime(NULL);
    new_inode.modify_time = ktime(NULL);
    new_inode.delete_time = 0;
    new_inode.group_id = 0;
    new_inode.hard_link_count = 1;
    new_inode.nbr_sectors = file_size / 512;
    new_inode.flags = 0;
    new_inode.os_value1;
    new_inode.generation = 0;
    new_inode.frag_addr;

    new_inode.dbp[0] = _ext2_find_free_blocks(dev);
    for(size_t i = 1; i < 12; i++)
    {
        new_inode.dbp[i] = 0;
    }
    new_inode.sibp = 0;
    new_inode.dibp = 0;
    new_inode.tibp = 0;

    _ext2_update_inode_table(dev, new_id_inode, &new_inode);

    return new_id_inode;
}

static uint32_t _ext2_get_entry_inode_idx(fs_device_t* dev, ext2_dir_entry* dir, const char* filename)
{
    // if file name is too large return 0
    if(strlen(filename) > 255)
    {
        return 0;
    }

    char name_buffer[255];
    void* dir_ptr = dir;
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;

    do
    {
        entry = dir_ptr;
        next_entry = dir_ptr + entry->entry_size;
        strcpy(&name_buffer, &entry->name);
        name_buffer[entry->name_length] = 0;
        if(entry->type != 0 && !strcmp(&entry->name, filename))
        {
            return entry->inode;
        }
        dir_ptr += entry->entry_size;
    } while(next_entry->inode);

    return next_entry->inode;
}

/*
    FILES
*/

static int _ext2_read(uint8_t* data, uint8_t* buffer, uint64_t offset, uint64_t len)
{
    memcpy(buffer, &data[offset], len);
    return 0;
}

static uint8_t _ext2_read_file(fs_device_t* dev, uint8_t* buffer, ext2_inode* inode)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;
    for(size_t i = 0; i < 12; i++)
    {
        if(inode->dbp[i] != 0)
        {
            dev->read(dev, buffer, inode->dbp[i] * fs_data->sec_per_block, fs_data->sec_per_block);
            buffer += 512 * fs_data->sec_per_block;
        }
    }

    uint32_t buf[4096 / 4];
    if(inode->sibp)
    {
        dev->read(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);
        for(size_t i = 0; i < (4096 / 4); i++)
        {
            if(buf[i] != 0)
            {
                dev->read(dev, buffer, buf[i] * fs_data->sec_per_block, fs_data->sec_per_block);
                buffer += 512 * fs_data->sec_per_block;
            }
        }
    }

    return 0;
}

static int _ext2_update_file(fs_device_t* dev, uint8_t* buffer, uint64_t offset, uint64_t len, ext2_inode* inode, uint32_t inode_id)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    /* TODO LATER must support at least 1 single indirect block pointer */
    uint32_t buf[4096 / 4];
    if(len > (12 * 4096) && inode->sibp == 0)
    {
        inode->sibp = _ext2_find_higher_half_free_blocks(dev);
    }

    if(inode->sibp != 0)
    {
        dev->read(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);
    }

    size_t i = 0;

    while(inode->size < len)
    {
        if(i < 12 && inode->dbp[i] == 0)
        {
            inode->dbp[i] = _ext2_find_free_blocks(dev);
            inode->size += EXT2_BLOCK_SIZE;
        } else if(i >= 12)
        {
            if(buf[i - 12] == 0)
            {
                buf[i - 12] = _ext2_find_free_blocks(dev);
                inode->size += EXT2_BLOCK_SIZE;
            }
        }

        if(++i == 1036)
        {
            break;
        }
    }

    if(i != 0)
    {
        inode->nbr_sectors = (inode->size / 512) + 8;
        if(i >= 12)
        {
            /* update the single indirect block pointer */
            dev->write(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);
            inode->nbr_sectors += 8;
        }
        _ext2_update_inode_table(dev, inode_id, inode);
    }

    for(i = 0; i < 12; i++)
    {
        if(inode->dbp[i] != 0)
        {
            dev->write(dev, buffer, inode->dbp[i] * fs_data->sec_per_block, fs_data->sec_per_block);
            buffer += 512 * fs_data->sec_per_block;
        }
    }

    i = 0;
    while(buf[i])
    {
        dev->write(dev, buffer, buf[i] * fs_data->sec_per_block, fs_data->sec_per_block);
        buffer += 512 * fs_data->sec_per_block;
        i++;
    }

    return 0;
}

/*
    DIRECTORIES
*/

static int _ext2_update_dir_entry(fs_device_t* dev, ext2_inode* dir_inode, ext2_dir_entry* dir)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    for(size_t i = 0; i < 12; i++)
    {
        if(dir_inode->dbp[i] != 0)
        {
            dev->write(dev, (void*)dir, dir_inode->dbp[i]  * fs_data->sec_per_block, 8);
        }
    }

    return 0;
}

static int _ext2_read_dir_entry(void* dir_ptr, entry_read_dir_entries* read_entries, const char* filename)
{
    char name_buffer[255];

    do
    {
        read_entries->entry = dir_ptr;
        read_entries->next_entry = dir_ptr + read_entries->entry->entry_size;
        strcpy(&name_buffer, &read_entries->entry->name);
        name_buffer[read_entries->entry->name_length] = 0;
        if(read_entries->entry->type != 0 && strcmp(&name_buffer[0], filename) == 0)
        {
            int scmp = strcmp(&name_buffer[0], filename);
            return 0;
        }

        dir_ptr += read_entries->entry->entry_size;
    } while(read_entries->next_entry->inode);

    return -1;
}

static int _ext2_initialize_dir(fs_device_t* dev, ext2_dir_entry* dir, ext2_inode* dir_inode, uint32_t dir_inode_id, 
    ext2_inode* prev_dir_inode, uint32_t prev_dir_inode_id)
{
    void* dir_ptr = dir;
    entry_read_dir_entries entries;
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;

    // check if the dir is already initialize;
    if(!_ext2_read_dir_entry(dir_ptr, &entries, "."))
    {
        return -1;
    }

    entry = dir_ptr;
    entry->inode = dir_inode_id;
    char* name = &entry->name;
    name[0] = '.';
    name[1] = 0;
    entry->name_length = 1;
    entry->type = EXT2_TYPE_DIRECTORY;
    entry->entry_size = 12;
    dir_inode->hard_link_count = 2;
    _ext2_update_inode_table(dev, dir_inode_id, dir_inode);

    entry = dir_ptr + entry->entry_size;
    entry->inode = prev_dir_inode_id;
    name = &entry->name;
    name[0] = '.';
    name[1] = '.';
    name[2] = 0;
    entry->name_length = 2;
    entry->type = EXT2_TYPE_DIRECTORY;
    entry->entry_size = 4096 - 12;
    prev_dir_inode->hard_link_count++;
    _ext2_update_inode_table(dev, prev_dir_inode_id, prev_dir_inode);

    _ext2_update_dir_entry(dev, dir_inode, dir);

    ext2_fs_data* fs_data = dev->fs->fs_data;
    fs_data->blk_grp_desc.num_of_dir++;
    _ext2_update_sb_and_blk_desc(dev, fs_data);

    return 0;
}

static uint32_t _ext2_create_new_dir_entry(fs_device_t* dev, ext2_dir_entry* dir, const char* filename, 
    ext2_inode* dir_inode, uint64_t size, enum ext2_dir_entry_type type)
{
    if(strlen(filename) > 255)
    {
        return 0;
    }
    
    void* dir_ptr = dir;
    entry_read_dir_entries entries;

    if(!_ext2_read_dir_entry(dir_ptr, &entries, filename))
    {
        return 0;
    }

    uint16_t test_length = EXT2_DIR_ENTRY_SIZE(entries.entry->name_length);
    uint16_t free_size = entries.entry->entry_size - test_length;
    uint16_t new_entry_size = EXT2_DIR_ENTRY_SIZE(strlen(filename));
    
    if(free_size > new_entry_size)
    {
        // update old entry
        entries.entry->entry_size -= free_size;
        char* name = &entries.entry->name;
        // new entry 
        ext2_dir_entry* new_entry = ((void*)entries.entry) + entries.entry->entry_size;
        new_entry->entry_size = free_size;
        new_entry->name_length = strlen(filename);
        new_entry->type = type;
        strcpy(&new_entry->name, filename);
        
        switch (type)
        {
        case EXT2_TYPE_DIRECTORY:
            new_entry->inode = _ext2_create_new_inode(dev, size, EXT2_MODE_DIR | EXT2_DEFAULT_DIR_ACCESS);
            break;
        case EXT2_TYPE_REGULAR_FILE:
            new_entry->inode = _ext2_create_new_inode(dev, size, EXT2_MODE_REG_FILE | EXT2_DEFAULT_FILE_ACCESS);
            break;
        
        default:
            break;
        }

        _ext2_update_dir_entry(dev, dir_inode, dir);
        return new_entry->inode;
    }

    return 0;
}

static uint32_t _ext2_find_file(fs_device_t* dev, char** path, size_t* index, uint8_t new)
{
    if(*index == 0)
    {
        return 2;
    }

    ext2_idata* root_itable = ext2_cache_search_inode(dev, 2);
    // 4096 not 512! blocks are 4096 bytes so 8 * 512 sectors.
    uint32_t allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
    char* buffer = kmalloc(allocsize);

    _ext2_read_file(dev, buffer, &root_itable->inode);

    entry_read_dir_entries entries;
    uint32_t inode_id = 0;
   
    size_t size = 0;
    uint8_t found = 0;
    while(*path)
    {
        found = 0;
        if(!_ext2_read_dir_entry(buffer, &entries, *path))
        {
            found = 1;
            root_itable = ext2_cache_search_inode(dev, entries.entry->inode);
            inode_id = entries.entry->inode;
            if(!EXT2_IS_DIRECTORY(root_itable->inode.mode))
            {
                break;
            }
            kfree(buffer);
            allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
            buffer = kmalloc(allocsize);
            _ext2_read_file(dev, buffer, &root_itable->inode);
            size++;
        }
        path++;
    }
    
    kfree(buffer);
    
    if(new)
    {
        if(size == 0)
        {
            // TODO return the cwd
            *index = 0;
            return 2;
        }
        // file already exist
        if(size == *index)
        {
            *index = 0;
            return 0;
        }
    } else if(found == 0)
    {
        return 0;
    }
    
    *index = size;
    return inode_id;
}

static uint8_t _ext2_list_dir(uint8_t* dir)
{
    char name_buffer[255];
    void* dir_ptr = dir;
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;
    uint32_t idx = 0;
    
    do
    {
        entry = dir_ptr;
        next_entry = dir_ptr + entry->entry_size;
        strcpy(&name_buffer, &entry->name);
        name_buffer[entry->name_length] = 0;
        dir_ptr += entry->entry_size;
        memset(&name_buffer, 0, 255);
    } while(next_entry->inode);

    return 0;
}

/*
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
                                        FILE SYSTEM FUNCTION
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
*/

/*
    REGULAR FILE
*/
static int ext2_open(fs_device_t* dev, char* filename, fs_fd* fd)
{
    size_t index;
    uint8_t from_root;
    char** path = _ext2_get_path(filename, '/', &index, &from_root);
    uint32_t file_inode_nbr = _ext2_find_file(dev, path, &index, 0);
    
    if(file_inode_nbr == 0)
    {
        return -1;
    }

    ext2_idata* file_inode = ext2_cache_search_inode(dev, file_inode_nbr);


    if(file_inode->open == 0)
    {
        kernel_debug_output(KDB_LVL_INFO, "FILE NOT IN CACHE %s\n", filename);
        uint8_t* buffer = fs_cache_get_new_buffer(file_inode->inode.size);
        _ext2_read_file(dev, buffer, &file_inode->inode);
        ext2_add_file_to_cache(filename, file_inode, buffer);
        file_inode->open = 1;
    }

    fs_cache_increase_ref(file_inode->file_id);
    fd->ftable_idx = file_inode->file_id;
    fd->offset = 0;
    fd->inode_nbr = file_inode->inode_nbr;

    vmfree(path);

    return 0;
}

static int ext2_close(fs_device_t* dev, fs_fd* fd)
{
    ext2_idata* file_inode = ext2_cache_search_inode(dev, fd->inode_nbr);
    ext2_close_file_from_cache(file_inode, fd);
    return 0;
}

static int ext2_stat(fs_device_t* dev, fs_fd* fd, fs_file_stat* stat)
{
    ext2_idata* file_inode = ext2_cache_search_inode(dev, fd->inode_nbr);

    stat->mode = file_inode->inode.mode;
    stat->size = file_inode->inode.size;
    stat->uid = file_inode->inode.user_id;
    stat->gid = file_inode->inode.group_id;
    stat->hard_link = file_inode->inode.hard_link_count;
    return 0;
}

static int ext2_touch(fs_device_t* dev, char* filename)
{
    int i = 0;
    int last_part = 0;

    size_t index;
    uint8_t from_root;
    char** path = _ext2_get_path(filename, '/', &index, &from_root);
    uint32_t file_inode_nbr = _ext2_find_file(dev, path, &index, 1);

    if(file_inode_nbr == 0)
    {
        return -1;
    }

    ext2_idata* root_itable = ext2_cache_search_inode(dev, file_inode_nbr);
    
    char* buffer = fs_cache_get_new_buffer(root_itable->inode.size);
    _ext2_read_file(dev, buffer, &root_itable->inode);
    _ext2_create_new_dir_entry(dev, (void*)buffer, path[index], &root_itable->inode, 0, EXT2_TYPE_REGULAR_FILE);

    // TODO FREE buffer
    vmfree(path);

    return 0;
}

static int ext2_read(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    fs_file* file = fs_cache_get_file(fd->ftable_idx);
    ext2_idata* inode =  ext2_cache_search_inode(dev, file->inode_nbr);

    if(inode->open == 0)
    {
        return -1;
    }

    _ext2_read(file->data, buffer, (uintptr_t)fd->offset, len);
    
    return 0;
}

static int ext2_write(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    // TODO
    return 0;
}

/*
    DIRECTORIES
*/

static int ext2_list(fs_device_t* dev, char* dirname, uint8_t* buffer)
{
    size_t index;
    uint8_t from_root;
    char** path = _ext2_get_path(dirname, '/', &index, &from_root);
    uint32_t inode = _ext2_find_file(dev, path, &index, 0);
    ext2_idata* root_itable = ext2_cache_search_inode(dev, inode);
    _ext2_read_file(dev, buffer, &root_itable->inode);
    _ext2_list_dir(buffer);

    vmfree(path);
    return 0;
}

static int ext2_mkdir(fs_device_t* dev, char* dirname)
{
    if(strlen(dirname) > 255)
    {
        return -1;
    }

    size_t index;
    uint8_t from_root;
    char** path = _ext2_get_path(dirname, '/', &index, &from_root);
    uint32_t prev_inode = _ext2_find_file(dev, path, &index, 1);
    
    if(prev_inode == 0)
    {
        return -1;
    }

    char* buffer = vmalloc(4096 * 1024);
    
    ext2_idata* root_itable = ext2_cache_search_inode(dev, prev_inode);
    _ext2_read_file(dev, buffer, &root_itable->inode);
    uint32_t inode = _ext2_create_new_dir_entry(dev, (void*)buffer, path[index], (void*)&root_itable, 4096, EXT2_TYPE_DIRECTORY);

    ext2_idata* file_inode = ext2_cache_search_inode(dev, inode);
    _ext2_read_file(dev, buffer, &file_inode->inode);
    _ext2_initialize_dir(dev, (void*)buffer, &file_inode->inode, inode, &root_itable->inode, prev_inode);

    vmfree(buffer);
    vmfree(path);
    return 0;
}

/*
    FILE SYSTEM
*/

int ext2_probe(fs_device_t* dev)
{
    ext2_superblock* sb = vmalloc(sizeof(ext2_superblock));
    
    /*
        READ SUPER BLOCK
    */
    KERNEL_LOG_INFO("read super block %p", sb);
    dev->read(dev, (void*)sb, 2, 2);

    if(sb->ext2_signature != EXT2_SIGNATURE)
    {
        KERNEL_LOG_FAIL("not ext 2 0%p 0%x", sb, &sb->ext2_signature);
        vmfree(sb);
        while(1){}
        return -1;
    }
    KERNEL_LOG_OK("fs is ext2");

    ext2_fs_data* fs_data = vmalloc(sizeof(ext2_fs_data));
    fs_data->block_size = (1024 << sb->block_size_hint);
    fs_data->sec_per_block = fs_data->block_size / 512;

    memcpy(&fs_data->sb, sb, sizeof(ext2_superblock));
    vmfree(sb);
    
    /*
        READ BLOCK GROUP DESCRIPTOR
    */

    uint8_t block_grp_loc = 1;
    ext2_block_group_descriptor* block_desc = vmalloc(512);
    if(fs_data->block_size == 1024)
    {
        block_grp_loc = 2;
    }
    dev->read(dev, (void*)block_desc, block_grp_loc * fs_data->sec_per_block, 1);

    memcpy(&fs_data->blk_grp_desc, block_desc, sizeof(ext2_block_group_descriptor));
    vmfree(block_desc);

    dev->fs = vmalloc(sizeof(file_system_t));
    dev->fs->probe = ext2_probe;
    dev->fs->open = ext2_open;
    dev->fs->close = ext2_close;
    dev->fs->stat = ext2_stat;
    dev->fs->read = ext2_read;
    dev->fs->write = ext2_write;
    dev->fs->touch = ext2_touch;
    dev->fs->list = ext2_list;
    dev->fs->mkdir = ext2_mkdir;
    dev->fs->fs_data = fs_data;

    return 0;
}