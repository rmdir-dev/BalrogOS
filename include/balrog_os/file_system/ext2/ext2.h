#pragma once

#include <stdint.h>
#include "balrog_os/file_system/filesystem.h"

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
/*  inode, entry_size, name_length and type : no entry can be shorter  */
#define EXT2_DIR_ENTRY_MIN              8

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
    // Extended superblock fields
    uint32_t first_inode;                       // First non-reserved inode in file system. (In versions < 1.0, this is fixed as 11)
    uint16_t inode_size;                        // Size of each inode structure in bytes. (In versions < 1.0, this is fixed as 128)
    uint16_t superblock_group;                  // Block group that this superblock is part of (if backup copy)
    /*
    Optional Feature Flags
    These are optional features for an implementation to support, but offer performance or reliability gains to implementations that do support them.

    Flag Value	Description
    0x0001	Preallocate some number of (contiguous?) blocks (see byte 205 in the superblock) to a directory when creating a new one (to reduce fragmentation?)
    0x0002	AFS server inodes exist
    0x0004	File system has a journal (Ext3)
    0x0008	Inodes have extended attributes
    0x0010	File system can resize itself for larger partitions
    0x0020	Directories use hash index
    */
    uint32_t feature_compat;                    // Optional features present (features that are not required to read or write, but usually result in a performance increase. see above
    /*
    These features if present on a file system are required to be supported by an implementation in order to correctly read from or write to the file system.

    Flag Value	Description
    0x0001	Compression is used
    0x0002	Directory entries contain a type field
    0x0004	File system needs to replay its journal
    0x0008	File system uses a journal device
    */
    uint32_t feature_incompat;                  // Required features present (features that are required to be supported to read or write. see above)
    /*
    These features, if present on a file system, are required in order for an implementation to write to the file system, but are not required to read from the file system.

    Flag Value	Description
    0x0001	Sparse superblocks and group descriptor tables
    0x0002	File system uses a 64-bit file size
    0x0004	Directory contents are stored in the form of a Binary Tree
    */
    uint32_t feature_ro_compat;                 // Features that if not supported, the volume must be mounted read-only see above)
    uint8_t  uuid[16];                          // File system ID (what is output by blkid)
    char     volume_name[16];                   // Volume name (C-style string: characters terminated by a 0 byte)
    char     last_mounted[64];                  // Path volume was last mounted to (C-style string: characters terminated by a 0 byte)
    /*
    start   end     size    description
    200	    203	    4	    Compression algorithms used (see Required features above)
    204	    204	    1	    Number of blocks to preallocate for files
    205	    205	    1	    Number of blocks to preallocate for directories
    206	    207	    2	    (Unused)
    208	    223	    16	    Journal ID (same style as the File system ID above)
    224	    227	    4	    Journal inode
    228	    231	    4	    Journal device
    232	    235	    4	    Head of orphan inode list
    236	    1023	X	    (Unused)
    */
    uint8_t  unused[824];                       // what is left of the 1024
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
    // filename
    const char* filename;
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