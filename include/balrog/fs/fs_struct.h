#pragma once
#include <stdint.h>

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

typedef struct _fs_file_stat
{
    // type and protection
    uint16_t mode;
    // file size
    uint32_t size;
    // user id
    uint16_t uid;
    // group id
    uint16_t gid;
    // hard link count
    uint16_t hard_link;
} fs_file_stat;