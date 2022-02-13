#pragma once

#include <stdint.h>
#include "BalrogOS/FileSystem/filesystem.h"

/*
    FILE SYSTEM STATE
    value   state
    1       file system is clean
    2       file system has errors

    ERROR HANDLING METHODS
    value   action
    1       ignore the error
    2       remount the file system as read only
    3       kernel panic

    CREATOR OPERATING SYSTEM IDs
    value   OS
    0       Linux
    1       GNU HURD
    2       MASIX
    3       FreeBSD
    4       Other "lites" (BSD4.4-Lite derivatives such as NetBS, OpenBSD, XNU/Darwinm, ...)
*/

#define EXT2_SIGNATURE 0xef53

/*
    if the name_len is not a multiple of 4. then the entry size is equal to :
        (((name_len / 4) + 1) * 4) + struct_size
    else 
        (((name_len / 4) + 0) * 4) + struct_size
*/
#define EXT2_DIR_ENTRY_SIZE(name_len) (name_len % 4) ? (((name_len / 4) + 1) * 4) + (sizeof(ext2_dir_entry) - 1) :\
                                                        (((name_len / 4)) * 4) + (sizeof(ext2_dir_entry) - 1)


#define EXT2_MODE_SOCKET        0xc000
#define EXT2_MODE_SYMB_LINK     0xa000
#define EXT2_MODE_REG_FILE      0x8000
#define EXT2_MODE_BLOCK_DEV     0x6000
#define EXT2_MODE_DIR           0x4000
#define EXT2_MODE_CHAR_DEV      0x2000
#define EXT2_MODE_FIFO          0x1000

#define EXT2_IS_DIRECTORY(imode)    ((imode & 0xf000) & EXT2_MODE_DIR)

/*
    BUGS 
    Pass 5: Checking group summary information
    Block bitmap differences:  -978 +2004 -12288 +12289
    Fix<y>? yes
    Free blocks count wrong for group #0 (29750, counted=28724).
    Fix<y>? yes
    Free blocks count wrong (1026, counted=28724).
    Fix<y>? yes
    Free inodes count wrong for group #0 (30740, counted=30738).
    Fix<y>? yes
    Directories count wrong for group #0 (2, counted=3).
    Fix<y>? yes
    Free inodes count wrong (30740, counted=30738).
    Fix<y>? yes

*/

typedef struct _ext2_superblock
{
    uint32_t inodes;                            // Total number of inodes in file system
    uint32_t blocks;                            // Total number of blocks in file system
    uint32_t su_blocks;                         // Number of blocks reserved for superuser (see offset 80)
    uint32_t unalloc_blocks;                    // Total number of unallocated blocks
    uint32_t unalloc_inodes;                    // Total number of unallocated inodes
    uint32_t superblock_id;                     // Block number of the block containing the superblock
    uint32_t block_size_hint;                   // log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)
    uint32_t fragment_size_hint;                // log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size)
    uint32_t blocks_per_grp;                    // Number of blocks in each block group
    uint32_t frag_per_grp;                      // Number of fragments in each block group
    uint32_t inode_per_grp;                     // Number of inodes in each block group
    uint32_t last_mount_time;                   // Last mount time (in POSIX time)
    uint32_t last_written_time;                 // Last written time (in POSIX time)
    uint16_t nbr_mounts_since_last_check;       // Number of times the volume has been mounted since its last consistency check (fsck)
    uint16_t max_nbr_mounts_before_check;       // Number of mounts allowed before a consistency check (fsck) must be done
    uint16_t ext2_signature;                    // Ext2 signature (0xef53), used to help confirm the presence of Ext2 on a volume
    uint16_t state;                             // File system state (see above)
    uint16_t op_on_error;                       // What to do when an error is detected (see above)
    uint16_t minor_version;                     // Minor portion of version (combine with Major portion below to construct full version field)
    uint32_t last_check;                        // POSIX time of last consistency check (fsck)
    uint32_t max_time_before_check;             // Interval (in POSIX time) between forced consistency checks (fsck)
    uint32_t os_id;                             // Operating system ID from which the filesystem on this volume was created (see above)
    uint32_t major_version;                     // Major portion of version (combine with Minor portion above to construct full version field)
    uint16_t user_id;                           // User ID that can use reserved blocks
    uint16_t group_id;                          // Group ID that can use reserved blocks
    uint8_t unused[940];                        // Use with Extended Superblock Fields (Major version >= 1)
} __attribute__((packed)) ext2_superblock;

typedef struct __ext2_block_group_descriptor
{
    uint32_t block_addr_of_block_usage_bitmap;  // Block address of block usage bitmap
    uint32_t block_addr_of_inode_usage_bitmap;  // Block address of inode usage bitmap
    uint32_t block_addr_of_inode_table;         // Starting block address of inode table
    uint16_t num_of_unalloc_block;              // Number of unallocated blocks in group
    uint16_t num_of_unalloc_inode;              // Number of unallocated inodes in group
    uint16_t num_of_dir;                        // Number of directories in group
    uint8_t unused[14];
} __attribute__((packed)) ext2_block_group_descriptor;

typedef struct _ext2_inode
{
    uint16_t mode;
    uint16_t user_id;
    uint32_t size;
    uint32_t access_time;
    uint32_t create_time;
    uint32_t modify_time;
    uint32_t delete_time;
    uint16_t group_id;
    uint16_t hard_link_count;
    uint32_t nbr_sectors;
    uint32_t flags;
    uint32_t os_value1;
    uint32_t dbp[12];           // Direct block pointer
    uint32_t sibp;              // singly indirect block pointer
    uint32_t dibp;              // doubly indirect block pointer
    uint32_t tibp;              // Triply indirect block pointer
    uint32_t generation;        // Generation number is use for network file sys
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t frag_addr;
    uint8_t os_value2[12];
} __attribute__((packed)) ext2_inode;

typedef struct _ext2_dir_entry
{
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_length;
    uint8_t type;
    char name;
} __attribute__((packed)) ext2_dir_entry;

typedef struct _ext2_fs_data
{
    ext2_superblock sb;
    ext2_block_group_descriptor blk_grp_desc;
    uint32_t block_size;
    uint32_t sec_per_block;
} __attribute__((packed)) ext2_fs_data;

typedef struct _ext2_idata
{
    // the inode nbr
    uint32_t inode_nbr;
    // the file inode
    ext2_inode inode;
    // 1 if opened else 0
    uint8_t open;
    // file id in the open file table
    uint32_t file_id;
} __attribute__((packed)) ext2_idata;

enum ext2_dir_entry_type
{
    EXT2_TYPE_UNKNOWN_TYPE = 0,
    EXT2_TYPE_REGULAR_FILE = 1,
    EXT2_TYPE_DIRECTORY = 2,
    EXT2_TYPE_CHARACTER_DEVICE = 3,
    EXT2_TYPE_BLOCK_DEVICE = 4,
    EXT2_TYPE_FIFO = 5,
    EXT2_TYPE_SOCKET = 6,
    EXT2_TYPE_SYMBOLIC_LINK = 7
};

int ext2_probe(fs_device_t* dev);