#include "balrog_os/file_system/ext2/ext2_cache/ext2_cache.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/tasking/tasking.h"
#include "klib/io/kprint.h"
#include "ext2_config.h"
#include "klib/ktime.h"

#include <unistd.h>
#include <string.h>

/*
    UTILITIES
*/

extern process* current_running;

typedef struct _entry_read_dir_entries
{
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;
} entry_read_dir_entries;


typedef struct __ext2_dir_walk_t
{
    void* ptr;
    ext2_dir_entry* entry;
    ext2_dir_entry* previous;
    char name[256];
} ext2_dir_walk_t;

static void __ext2_entry_name(ext2_dir_entry* entry, char* out)
{
    memcpy(out, &entry->name, entry->name_length);
    out[entry->name_length] = 0;
}

static void __ext2_walk_begin(ext2_dir_walk_t* walk, void* dir)
{
    walk->ptr = dir;
    walk->entry = 0;
    walk->previous = 0;
}

static int __ext2_walk_next(ext2_dir_walk_t* walk)
{
    if(walk->entry != 0 && ((ext2_dir_entry*)walk->ptr)->inode == 0)
    {
        return 0;
    }

    ext2_dir_entry* entry = walk->ptr;

    if(entry->entry_size < EXT2_DIR_ENTRY_MIN)
    {
        return 0;
    }

    walk->previous = walk->entry;
    walk->entry = entry;
    walk->ptr += entry->entry_size;

    __ext2_entry_name(entry, walk->name);

    return 1;
}

static char** __ext2_get_path(char* src, const char delimiter, size_t* out_size, uint8_t* from_root)
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

static inline uint32_t __ext2_find_free_bitmap(fs_device_t* dev, size_t bitmap_size, size_t start_idx, uint32_t sec_per_block, size_t map_idx_start)
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
    /* every caller tests the result as a block id, 0 is the free one */
    return 0;
}

static int __ext2_update_sb_and_blk_desc(fs_device_t* dev, ext2_fs_data* fs_data)
{
    dev->write(dev, (void*)&fs_data->sb, 2, 2);

    /*  the same block the probe reads it from : the one after the superblock,
        which is block 2 for a 1024 byte block and block 1 for anything else.  */
    uint32_t block_grp_loc = (fs_data->block_size == 1024) ? 2 : 1;

    dev->write(dev, (void*)&fs_data->blk_grp_desc, block_grp_loc * fs_data->sec_per_block, 1);

    return 0;
}

/*
    BLOCKS
*/

static int __ext2_free_alloc_block(fs_device_t* dev, uint32_t block_id)
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
    /*  the sector is taken from the start of the block usage bitmap, and the
        byte is the one inside that sector. the id is 1 based like the one
        __ext2_find_free_bitmap() hands out.
    */
    uint32_t map_sector = ((block_id - 1) / 8) / 512;
    uint32_t map_byte = ((block_id - 1) / 8) % 512;
    uint32_t map_lba = (fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap * fs_data->sec_per_block) + map_sector;

    dev->read(dev, (void*)&block_bitmap, map_lba, 1);
    block_bitmap[map_byte] &= (0xff ^ (1 << ((block_id - 1) % 8)));
    dev->write(dev, (void*)&block_bitmap, map_lba, 1);

    fs_data->sb.unalloc_blocks++;
    fs_data->blk_grp_desc.num_of_unalloc_block++;
    __ext2_update_sb_and_blk_desc(dev, fs_data);

    return 0;
}

/*  the inode bitmap works exactly like the block one, it just lives at
    another place and counts against the inode counters.
*/
static int __ext2_free_alloc_inode(fs_device_t* dev, uint32_t inode_id)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;
    uint8_t inode_bitmap[512];

    uint32_t map_sector = ((inode_id - 1) / 8) / 512;
    uint32_t map_byte = ((inode_id - 1) / 8) % 512;
    uint32_t map_lba = (fs_data->blk_grp_desc.block_addr_of_inode_usage_bitmap * fs_data->sec_per_block) + map_sector;

    dev->read(dev, (void*)&inode_bitmap, map_lba, 1);
    inode_bitmap[map_byte] &= (0xff ^ (1 << ((inode_id - 1) % 8)));
    dev->write(dev, (void*)&inode_bitmap, map_lba, 1);

    fs_data->sb.unalloc_inodes++;
    fs_data->blk_grp_desc.num_of_unalloc_inode++;
    __ext2_update_sb_and_blk_desc(dev, fs_data);

    return 0;
}

static uint32_t __ext2_find_higher_half_free_blocks(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = __ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap, 
            fs_data->sec_per_block, block_bitmap_size / 2);
    if(block_id)
    {
        fs_data->sb.unalloc_blocks--;
        fs_data->blk_grp_desc.num_of_unalloc_block--;
        __ext2_update_sb_and_blk_desc(dev, fs_data);

        kernel_debug_output(KDB_LVL_VERBOSE, "ext2 : block %d allocated, %d left", block_id, fs_data->sb.unalloc_blocks);
        return block_id;
    }

    kernel_debug_output(KDB_LVL_ERROR, "ext2 : no free block left, %d claimed free in the superblock", fs_data->sb.unalloc_blocks);
    return block_id;
}

static uint32_t __ext2_find_free_blocks(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = __ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap, fs_data->sec_per_block, 0);
    if(block_id)
    {
        fs_data->sb.unalloc_blocks--;
        fs_data->blk_grp_desc.num_of_unalloc_block--;
        __ext2_update_sb_and_blk_desc(dev, fs_data);

        kernel_debug_output(KDB_LVL_VERBOSE, "ext2 : block in the higher half %d allocated, %d left", block_id, fs_data->sb.unalloc_blocks);
        return block_id;
    }

    kernel_debug_output(KDB_LVL_ERROR, "ext2 : no free block in the higher half left, %d claimed free in the superblock", fs_data->sb.unalloc_blocks);
    return block_id;
}

/*
    INODES
*/

static uint32_t __ext2_find_free_inode(fs_device_t* dev)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    size_t block_bitmap_size = fs_data->sb.blocks / 8;

    uint32_t block_id = __ext2_find_free_bitmap(dev, block_bitmap_size, fs_data->blk_grp_desc.block_addr_of_inode_usage_bitmap, fs_data->sec_per_block, 0);
    if(block_id)
    {
        fs_data->sb.unalloc_inodes--;
        fs_data->blk_grp_desc.num_of_unalloc_inode--;
        __ext2_update_sb_and_blk_desc(dev, fs_data);

        kernel_debug_output(KDB_LVL_VERBOSE, "ext2 : inode %d allocated, %d left", block_id, fs_data->sb.unalloc_inodes);
        return block_id;
    }

    kernel_debug_output(KDB_LVL_ERROR, "ext2 : no free inode left, %d claimed free in the superblock", fs_data->sb.unalloc_inodes);
    return block_id;
}

static int __ext2_update_inode_table(fs_device_t* dev, uint32_t inode_idx, ext2_inode* inode)
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

static uint32_t __ext2_create_new_inode(fs_device_t* dev, uint32_t file_size, uint16_t mode)
{
    uint32_t new_id_inode = __ext2_find_free_inode(dev);
    
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

    new_inode.dbp[0] = __ext2_find_free_blocks(dev);
    for(size_t i = 1; i < 12; i++)
    {
        new_inode.dbp[i] = 0;
    }
    new_inode.sibp = 0;
    new_inode.dibp = 0;
    new_inode.tibp = 0;

    __ext2_update_inode_table(dev, new_id_inode, &new_inode);

    return new_id_inode;
}

static uint32_t __ext2_get_entry_inode_idx(fs_device_t* dev, ext2_dir_entry* dir, const char* filename)
{
    // if file name is too large return 0
    if(strlen(filename) > 255)
    {
        return 0;
    }

    ext2_dir_walk_t walk;
    __ext2_walk_begin(&walk, dir);

    while(__ext2_walk_next(&walk))
    {
        if(walk.entry->type != 0 && !strcmp(walk.name, filename))
        {
            return walk.entry->inode;
        }
    }

    return 0;
}

/*
    FILES
*/

static int __ext2_read(uint8_t* data, uint8_t* buffer, uint64_t offset, uint64_t len)
{
    memcpy(buffer, &data[offset], len);
    return 0;
}

static int __ext2_write(uint8_t* data, uint8_t* buffer, uint64_t offset, uint64_t len)
{
    memcpy(&data[offset], buffer, len);
    return 0;
}

static uint8_t __ext2_read_file(fs_device_t* dev, uint8_t* buffer, ext2_inode* inode)
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

static int __ext2_update_file(fs_device_t* dev, uint8_t* buffer, uint64_t offset, uint64_t len, ext2_inode* inode, uint32_t inode_id)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    /*  buf holds the single indirect block. it is only read from the disk
        when the inode already has one, so it has to start empty. else the
        block numbers below are whatever was left on the stack.
    */
    uint32_t buf[EXT2_SIBP_ENTRIES];
    memset(&buf, 0, sizeof(buf));

    /* TODO : a file never shrinks it is only ever written over or grown */
    if(inode->size < offset + len)
    {
        inode->size = offset + len;
    }

    /*  the file needs one block per EXT2_BLOCK_SIZE bytes. the first 12 are
        the direct ones, the rest is held by the single indirect block.
    */
    size_t needed = (inode->size + (EXT2_BLOCK_SIZE - 1)) / EXT2_BLOCK_SIZE;

    if(needed > EXT2_MAX_BLOCKS)
    {
        needed = EXT2_MAX_BLOCKS;
    }

    uint8_t new_sibp = 0;

    if(needed > 12 && inode->sibp == 0)
    {
        inode->sibp = __ext2_find_higher_half_free_blocks(dev);
        new_sibp = 1;
    }

    if(inode->sibp != 0 && !new_sibp)
    {
        dev->read(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);
    }

    for(size_t i = 0; i < needed; i++)
    {
        if(i < 12)
        {
            if(inode->dbp[i] == 0)
            {
                inode->dbp[i] = __ext2_find_free_blocks(dev);
            }
        } else if(inode->sibp != 0)
        {
            if(buf[i - 12] == 0)
            {
                buf[i - 12] = __ext2_find_free_blocks(dev);
            }
        }
    }

    inode->nbr_sectors = needed * fs_data->sec_per_block;

    if(inode->sibp != 0)
    {
        /* update the single indirect block pointer */
        dev->write(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);
        inode->nbr_sectors += fs_data->sec_per_block;
    }

    __ext2_update_inode_table(dev, inode_id, inode);

    /* writing only what is necessary instead of the whole file each time. */
    size_t first = offset / EXT2_BLOCK_SIZE;
    size_t last = (offset + len + (EXT2_BLOCK_SIZE - 1)) / EXT2_BLOCK_SIZE;

    if(last > needed)
    {
        last = needed;
    }

    for(size_t i = first; i < last; i++)
    {
        uint32_t block_id = (i < 12) ? inode->dbp[i] : buf[i - 12];

        if(block_id == 0)
        {
            continue;
        }

        dev->write(dev, buffer + (i * EXT2_BLOCK_SIZE), block_id * fs_data->sec_per_block, fs_data->sec_per_block);
    }

    return 0;
}

/*
    DIRECTORIES
*/

static int __ext2_update_dir_entry(fs_device_t* dev, ext2_inode* dir_inode, ext2_dir_entry* dir)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;
    void* dir_ptr = dir;

    for(size_t i = 0; i < 12; i++)
    {
        if(dir_inode->dbp[i] != 0)
        {
            dev->write(dev, dir_ptr, dir_inode->dbp[i]  * fs_data->sec_per_block, fs_data->sec_per_block);
            /*  the directory is one contiguous buffer, it is walked block by
                block the same way __ext2_read_file() filled it. without this
                every block of the directory gets a copy of the first one.
            */
            dir_ptr += 512 * fs_data->sec_per_block;
        }
    }

    /*  a directory larger than 12 blocks keeps the rest of them in its
        single indirect block, __ext2_read_file() reads them the same way.
    */
    if(dir_inode->sibp != 0)
    {
        uint32_t buf[EXT2_SIBP_ENTRIES];
        dev->read(dev, (void*)&buf, dir_inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);

        for(size_t i = 0; i < EXT2_SIBP_ENTRIES; i++)
        {
            if(buf[i] != 0)
            {
                dev->write(dev, dir_ptr, buf[i] * fs_data->sec_per_block, fs_data->sec_per_block);
                dir_ptr += 512 * fs_data->sec_per_block;
            }
        }
    }

    return 0;
}

static int __ext2_read_dir_entry(void* dir_ptr, entry_read_dir_entries* read_entries, const char* filename)
{
    ext2_dir_walk_t walk;
    __ext2_walk_begin(&walk, dir_ptr);

    read_entries->entry = dir_ptr;
    read_entries->next_entry = dir_ptr;

    while(__ext2_walk_next(&walk))
    {
        read_entries->entry = walk.entry;
        read_entries->next_entry = walk.ptr;

        if(walk.entry->type != 0 && strcmp(walk.name, filename) == 0)
        {
            return 0;
        }
    }

    return -1;
}

static int __ext2_initialize_dir(fs_device_t* dev, ext2_dir_entry* dir, ext2_inode* dir_inode, uint32_t dir_inode_id, 
    ext2_inode* prev_dir_inode, uint32_t prev_dir_inode_id)
{
    void* dir_ptr = dir;
    entry_read_dir_entries entries;
    ext2_dir_entry* entry;
    ext2_dir_entry* next_entry;

    // check if the dir is already initialize;
    if(!__ext2_read_dir_entry(dir_ptr, &entries, "."))
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
    __ext2_update_inode_table(dev, dir_inode_id, dir_inode);

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
    __ext2_update_inode_table(dev, prev_dir_inode_id, prev_dir_inode);

    __ext2_update_dir_entry(dev, dir_inode, dir);

    ext2_fs_data* fs_data = dev->fs->fs_data;
    fs_data->blk_grp_desc.num_of_dir++;
    __ext2_update_sb_and_blk_desc(dev, fs_data);

    return 0;
}

static uint32_t __ext2_create_new_dir_entry(fs_device_t* dev, ext2_dir_entry* dir, const char* filename, 
    ext2_inode* dir_inode, uint64_t size, enum ext2_dir_entry_type type)
{
    if(strlen(filename) > 255)
    {
        kernel_debug_output(KDB_LVL_ERROR, "ext2 : '%s' is longer than 255", filename);
        return 0;
    }
    
    void* dir_ptr = dir;
    entry_read_dir_entries entries;

    if(!__ext2_read_dir_entry(dir_ptr, &entries, filename))
    {
        kernel_debug_output(KDB_LVL_ERROR, "ext2 : '%s' already exists", filename);
        return 0;
    }

    uint16_t test_length = EXT2_DIR_ENTRY_SIZE(entries.entry->name_length);
    uint16_t free_size = entries.entry->entry_size - test_length;
    uint16_t new_entry_size = EXT2_DIR_ENTRY_SIZE(strlen(filename));

    kernel_debug_output(KDB_LVL_INFO, "ext2 : last entry inode %d size %d name_len %d, free %d, need %d",
            entries.entry->inode,
            entries.entry->entry_size,
            entries.entry->name_length,
            free_size,
            new_entry_size
        );

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
            new_entry->inode = __ext2_create_new_inode(dev, size, EXT2_MODE_DIR | EXT2_DEFAULT_DIR_ACCESS);
            break;
        case EXT2_TYPE_REGULAR_FILE:
            new_entry->inode = __ext2_create_new_inode(dev, size, EXT2_MODE_REG_FILE | EXT2_DEFAULT_FILE_ACCESS);
            break;
        
        default:
            break;
        }

        __ext2_update_dir_entry(dev, dir_inode, dir);
        return new_entry->inode;
    }

    kernel_debug_output(KDB_LVL_ERROR, "ext2 : no room for '%s' : %d free, %d needed", filename, free_size, new_entry_size);
    return 0;
}

/*  a path that does not start with a '/' is relative to the working
    directory of the running process. the stored path is walked from the root
    to get the inode it stands for, and anything that can't be walked falls
    back to the root directory.
*/
static uint32_t __ext2_get_start_inode(fs_device_t* dev, uint8_t from_root)
{
    if(from_root || current_running == 0 || current_running->cwd == 0)
    {
        return EXT2_ROOT_INODE;
    }

    char* cwd = strdup(current_running->cwd);

    if (cwd == 0)
    {
        return -1;
    }

    size_t cwd_index;
    uint8_t cwd_from_root;
    char** cwd_path = __ext2_get_path(cwd, '/', &cwd_index, &cwd_from_root);
    uint32_t inode_id = EXT2_ROOT_INODE;

    if(cwd_path == 0)
    {
        vmfree(cwd);
        return EXT2_ROOT_INODE;
    }

    ext2_idata* itable = ext2_cache_search_inode(dev, EXT2_ROOT_INODE);
    char** part = cwd_path;

    while(*part)
    {
        // 4096 not 512! blocks are 4096 bytes so 8 * 512 sectors.
        uint32_t allocsize = itable->inode.size + (4096 - (itable->inode.size % 4096));
        char* buffer = kmalloc(allocsize);
        entry_read_dir_entries entries;

        __ext2_read_file(dev, buffer, &itable->inode);

        if(__ext2_read_dir_entry(buffer, &entries, *part))
        {
            kfree(buffer);
            inode_id = EXT2_ROOT_INODE;
            break;
        }

        inode_id = entries.entry->inode;
        kfree(buffer);

        itable = ext2_cache_search_inode(dev, inode_id);

        if(!EXT2_IS_DIRECTORY(itable->inode.mode))
        {
            inode_id = EXT2_ROOT_INODE;
            break;
        }

        part++;
    }

    vmfree(cwd_path);
    vmfree(cwd);

    return inode_id;
}

/*  give every block the file holds back to the bitmap, the single indirect
    block included since it is a block of its own.
*/
static int __ext2_free_file_blocks(fs_device_t* dev, ext2_inode* inode)
{
    ext2_fs_data* fs_data = dev->fs->fs_data;

    for(size_t i = 0; i < 12; i++)
    {
        if(inode->dbp[i] != 0)
        {
            __ext2_free_alloc_block(dev, inode->dbp[i]);
            inode->dbp[i] = 0;
        }
    }

    if(inode->sibp != 0)
    {
        uint32_t buf[EXT2_SIBP_ENTRIES];
        dev->read(dev, (void*)&buf, inode->sibp * fs_data->sec_per_block, fs_data->sec_per_block);

        for(size_t i = 0; i < EXT2_SIBP_ENTRIES; i++)
        {
            if(buf[i] != 0)
            {
                __ext2_free_alloc_block(dev, buf[i]);
            }
        }

        __ext2_free_alloc_block(dev, inode->sibp);
        inode->sibp = 0;
    }

    return 0;
}

/*  an entry is removed by handing its room over to the one before it. the
    very first entry of the block has nothing before it, so it is only
    emptied out and keeps its size.
*/
static uint32_t __ext2_remove_dir_entry(void* dir_ptr, const char* filename)
{
    ext2_dir_walk_t walk;
    __ext2_walk_begin(&walk, dir_ptr);

    while(__ext2_walk_next(&walk))
    {
        if(walk.entry->type != 0 && strcmp(walk.name, filename) == 0)
        {
            uint32_t inode_id = walk.entry->inode;

            if(walk.previous != 0)
            {
                walk.previous->entry_size += walk.entry->entry_size;
            } else
            {
                walk.entry->inode = 0;
                walk.entry->name_length = 0;
                walk.entry->type = EXT2_TYPE_UNKNOWN_TYPE;
            }

            return inode_id;
        }
    }

    return 0;
}

/*  a directory can only go away once it holds nothing but "." and "..".
*/
static uint8_t __ext2_dir_is_empty(void* dir_ptr)
{
    ext2_dir_walk_t walk;
    __ext2_walk_begin(&walk, dir_ptr);

    while(__ext2_walk_next(&walk))
    {
        if(walk.entry->type != 0 && strcmp(walk.name, ".") != 0 && strcmp(walk.name, "..") != 0)
        {
            return 0;
        }
    }

    return 1;
}

static uint32_t __ext2_find_file(fs_device_t* dev, char** path, size_t* index, uint8_t new, uint8_t from_root)
{
    uint32_t start_inode = __ext2_get_start_inode(dev, from_root);

    if(*index == 0)
    {
        return start_inode;
    }

    ext2_idata* root_itable = ext2_cache_search_inode(dev, start_inode);
    // 4096 not 512! blocks are 4096 bytes so 8 * 512 sectors.
    uint32_t allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
    char* buffer = kmalloc(allocsize);

    __ext2_read_file(dev, buffer, &root_itable->inode);

    entry_read_dir_entries entries;
    uint32_t inode_id = 0;

    /* we start reading from the parent directory */
    uint32_t parent_id = start_inode;

    size_t size = 0;
    uint8_t found = 0;
    while(*path)
    {
        found = 0;
        if(!__ext2_read_dir_entry(buffer, &entries, *path))
        {
            root_itable = ext2_cache_search_inode(dev, entries.entry->inode);
            inode_id = entries.entry->inode;
            char *next_path = *(path + 1);
            if(!next_path)
            {
                found = 1;
                break;
            }

            // If not a directory ->  cannot entre so exit.
            if(!EXT2_IS_DIRECTORY(root_itable->inode.mode))
            {
                break;
            }
            kfree(buffer);
            allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
            buffer = kmalloc(allocsize);
            __ext2_read_file(dev, buffer, &root_itable->inode);
            parent_id = inode_id;
            size++;
        }
        path++;
    }
    
    kfree(buffer);
    
    if(new)
    {
        if(size == 0)
        {
            *index = 0;
            return start_inode;
        }
        // file already exist
        if(size == *index)
        {
            *index = 0;
            return 0;
        }

        *index = size;
        return parent_id;
    } else if(found == 0)
    {
        return 0;
    }
    
    *index = size;
    return inode_id;
}

static uint8_t __ext2_list_dir(uint8_t* dir)
{
    ext2_dir_walk_t walk;
    __ext2_walk_begin(&walk, dir);

    while(__ext2_walk_next(&walk))
    {
    }

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
    char* cwd = strdup(filename);

    if (cwd == 0)
    {
        return -1;
    }

    char** path = __ext2_get_path(cwd, '/', &index, &from_root);
    uint32_t file_inode_nbr = __ext2_find_file(dev, path, &index, 0, from_root);
    
    if(file_inode_nbr == 0)
    {
        vmfree(cwd);
        vmfree(path);
        return -1;
    }

    ext2_idata* file_inode = ext2_cache_search_inode(dev, file_inode_nbr);

    if(file_inode->open == 0)
    {
        kernel_debug_output(KDB_LVL_INFO, "FILE NOT IN CACHE %s\n", filename);
        uint8_t* buffer = fs_cache_get_new_buffer(file_inode->inode.size);
        __ext2_read_file(dev, buffer, &file_inode->inode);
        ext2_add_file_to_cache(filename, file_inode, buffer);
        file_inode->open = 1;
    }

    fs_cache_increase_ref(file_inode->file_id);
    fd->ftable_idx = file_inode->file_id;
    fd->offset = 0;
    fd->inode_nbr = file_inode->inode_nbr;

    vmfree(cwd);
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

static int ext2_create(fs_device_t* dev, char* filename, uint64_t size)
{
    int i = 0;
    int last_part = 0;

    size_t index;
    uint8_t from_root;
    char* cwd = strdup(filename);

    if (cwd == 0)
    {
        return -1;
    }

    char** path = __ext2_get_path(cwd, '/', &index, &from_root);
    uint32_t file_inode_nbr = __ext2_find_file(dev, path, &index, 1, from_root);

    if(file_inode_nbr == 0)
    {
        vmfree(cwd);
        vmfree(path);
        return -1;
    }

    ext2_idata* root_itable = ext2_cache_search_inode(dev, file_inode_nbr);
    
    /*  the directory is only needed while the entry is added, so it is taken
        from the kernel heap the way __ext2_find_file() does it. the file
        system cache has no way to give a buffer back.
    */
    uint32_t allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
    char* buffer = kmalloc(allocsize);

    __ext2_read_file(dev, buffer, &root_itable->inode);
    uint32_t inode = __ext2_create_new_dir_entry(dev, (void*)buffer, path[index], &root_itable->inode, size, EXT2_TYPE_REGULAR_FILE);

    kfree(buffer);
    vmfree(cwd);
    vmfree(path);

    if (root_itable->open != 0)
    {
        ext2_invalidate_cache(root_itable);
    }

    return inode ? 0 : -1;
}

static int ext2_read(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    fs_file* file = fs_cache_get_file(fd->ftable_idx);
    ext2_idata* inode =  ext2_cache_search_inode(dev, file->inode_nbr);

    if(inode->open == 0)
    {
        return -1;
    }

    __ext2_read(file->data, buffer, (uintptr_t)fd->offset, len);
    
    return 0;
}

static int ext2_write(fs_device_t* dev, uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    fs_file* file = fs_cache_get_file(fd->ftable_idx);
    ext2_idata* inode =  ext2_cache_search_inode(dev, file->inode_nbr);

    if(inode->open == 0)
    {
        return -1;
    }

    /*  the cached copy of the file only covers inode.size bytes, the pages
        past it are not mapped. growing a file needs the file system cache
        to hand out a larger buffer first.
    */
    if(((uintptr_t)fd->offset + len) > file->size)
    {
        return -1;
    }

    /* update the cached copy, then push the whole file back to the disk */
    __ext2_write(file->data, buffer, (uintptr_t)fd->offset, len);
    __ext2_update_file(dev, file->data, (uintptr_t)fd->offset, len, &inode->inode, inode->inode_nbr);

    return 0;
}

/*  remove a name from its directory and free everything the inode it points
    to was holding. dir_type tells a file apart from a directory, an empty
    directory also has to be taken out of the parent link count.
*/
static int __ext2_remove(fs_device_t* dev, char* filename, enum ext2_dir_entry_type type)
{
    size_t index;
    uint8_t from_root;
    char* cwd = strdup(filename);

    if (cwd == 0)
    {
        return -1;
    }

    char** path = __ext2_get_path(cwd, '/', &index, &from_root);
    uint32_t dir_inode_nbr = __ext2_find_file(dev, path, &index, 1, from_root);

    if(dir_inode_nbr == 0 || path == 0)
    {
        vmfree(cwd);
        vmfree(path);
        return -1;
    }

    ext2_idata* dir_itable = ext2_cache_search_inode(dev, dir_inode_nbr);

    // 4096 not 512! blocks are 4096 bytes so 8 * 512 sectors.
    uint32_t allocsize = dir_itable->inode.size + (4096 - (dir_itable->inode.size % 4096));
    char* buffer = kmalloc(allocsize);

    __ext2_read_file(dev, buffer, &dir_itable->inode);

    entry_read_dir_entries entries;

    if(__ext2_read_dir_entry(buffer, &entries, path[index]))
    {
        kfree(buffer);
        vmfree(path);
        vmfree(cwd);
        return -1;
    }

    if(entries.entry->type != type)
    {
        kfree(buffer);
        vmfree(path);
        vmfree(cwd);
        return -1;
    }

    uint32_t inode_nbr = entries.entry->inode;
    ext2_idata* itable = ext2_cache_search_inode(dev, inode_nbr);

    if(type == EXT2_TYPE_DIRECTORY)
    {
        uint32_t dir_allocsize = itable->inode.size + (4096 - (itable->inode.size % 4096));
        char* dir_buffer = kmalloc(dir_allocsize);

        __ext2_read_file(dev, dir_buffer, &itable->inode);

        if(!__ext2_dir_is_empty(dir_buffer))
        {
            kfree(dir_buffer);
            kfree(buffer);
            vmfree(path);
            vmfree(cwd);
            return -1;
        }

        kfree(dir_buffer);
    }

    if(__ext2_remove_dir_entry(buffer, path[index]) == 0)
    {
        kfree(buffer);
        vmfree(path);
        vmfree(cwd);
        return -1;
    }

    __ext2_free_file_blocks(dev, &itable->inode);

    itable->inode.hard_link_count = 0;
    itable->inode.delete_time = ktime(NULL);
    itable->inode.size = 0;
    itable->inode.nbr_sectors = 0;
    __ext2_update_inode_table(dev, inode_nbr, &itable->inode);
    __ext2_free_alloc_inode(dev, inode_nbr);
    ext2_cache_delete_inode(inode_nbr);

    /*  ".." inside the directory counted as a link on its parent, and the
        block group keeps a directory count of its own.
    */
    if(type == EXT2_TYPE_DIRECTORY)
    {
        ext2_fs_data* fs_data = dev->fs->fs_data;

        dir_itable->inode.hard_link_count--;
        __ext2_update_inode_table(dev, dir_inode_nbr, &dir_itable->inode);
        fs_data->blk_grp_desc.num_of_dir--;
        __ext2_update_sb_and_blk_desc(dev, fs_data);
    }

    __ext2_update_dir_entry(dev, &dir_itable->inode, (void*)buffer);

    kfree(buffer);
    vmfree(path);
    vmfree(cwd);

    return 0;
}

static int ext2_unlink(fs_device_t* dev, char* filename)
{
    return __ext2_remove(dev, filename, EXT2_TYPE_REGULAR_FILE);
}

/*
    DIRECTORIES
*/

static int ext2_rmdir(fs_device_t* dev, char* dirname)
{
    return __ext2_remove(dev, dirname, EXT2_TYPE_DIRECTORY);
}

static int ext2_list(fs_device_t* dev, char* dirname, uint8_t* buffer)
{
    size_t index;
    uint8_t from_root;
    char* cwd = strdup(dirname);

    if (cwd == 0)
    {
        return -1;
    }

    char** path = __ext2_get_path(cwd, '/', &index, &from_root);
    uint32_t inode = __ext2_find_file(dev, path, &index, 0, from_root);
    ext2_idata* root_itable = ext2_cache_search_inode(dev, inode);
    __ext2_read_file(dev, buffer, &root_itable->inode);
    __ext2_list_dir(buffer);

    vmfree(path);
    vmfree(cwd);
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
    char* cwd = strdup(dirname);

    if (cwd == 0)
    {
        return -1;
    }

    char** path = __ext2_get_path(cwd, '/', &index, &from_root);
    uint32_t prev_inode = __ext2_find_file(dev, path, &index, 1, from_root);
    
    if(prev_inode == 0)
    {
        vmfree(path);
        vmfree(cwd);
        return -1;
    }
    
    ext2_idata* root_itable = ext2_cache_search_inode(dev, prev_inode);
    uint32_t allocsize = root_itable->inode.size + (4096 - (root_itable->inode.size % 4096));
    char* buffer = vmalloc(allocsize);
    __ext2_read_file(dev, buffer, &root_itable->inode);
    uint32_t inode = __ext2_create_new_dir_entry(dev, (void*)buffer, path[index], &root_itable->inode, 4096, EXT2_TYPE_DIRECTORY);

    if(inode == 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "ext2_mkdir() : could not create '%s'", path[index]);
        vmfree(buffer);
        vmfree(path);
        vmfree(cwd);
        return -1;
    }

    ext2_idata* file_inode = ext2_cache_search_inode(dev, inode);
    __ext2_read_file(dev, buffer, &file_inode->inode);
    int init = __ext2_initialize_dir(dev, (void*)buffer, &file_inode->inode, inode, &root_itable->inode, prev_inode);

    if (init != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "ext2_mkdir() : '%s' created but not initialized", path[index]);
    }

    vmfree(buffer);
    vmfree(path);
    vmfree(cwd);

    if (root_itable->open != 0)
    {
        ext2_invalidate_cache(root_itable);
    }

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
        kernel_debug_output(KDB_LVL_ERROR, "ext2 : signature 0%x is not 0%x, not ext2",
                sb->ext2_signature, EXT2_SIGNATURE);
        vmfree(sb);
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

    kernel_debug_output(KDB_LVL_INFO, "ext2 : block size %d, %d sectors a block, group descriptor at block %d",
            fs_data->block_size, fs_data->sec_per_block, block_grp_loc);
    kernel_debug_output(KDB_LVL_INFO, "ext2 : %d blocks, %d free, %d inodes, %d free",
            fs_data->sb.blocks, fs_data->sb.unalloc_blocks,
            fs_data->sb.inodes, fs_data->sb.unalloc_inodes);
    kernel_debug_output(KDB_LVL_INFO, "ext2 : block usage bitmap at %d, inode usage bitmap at %d, inode table at %d",
            fs_data->blk_grp_desc.block_addr_of_block_usage_bitmap,
            fs_data->blk_grp_desc.block_addr_of_inode_usage_bitmap,
            fs_data->blk_grp_desc.block_addr_of_inode_table);

    dev->fs = vmalloc(sizeof(file_system_t));
    dev->fs->probe = ext2_probe;
    dev->fs->open = ext2_open;
    dev->fs->close = ext2_close;
    dev->fs->stat = ext2_stat;
    dev->fs->read = ext2_read;
    dev->fs->write = ext2_write;
    dev->fs->create = ext2_create;
    dev->fs->list = ext2_list;
    dev->fs->mkdir = ext2_mkdir;
    dev->fs->unlink = ext2_unlink;
    dev->fs->rmdir = ext2_rmdir;
    dev->fs->fs_data = fs_data;

    return 0;
}