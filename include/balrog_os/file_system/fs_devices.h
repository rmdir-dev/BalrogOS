#pragma once

#include "klib/data_structure/list.h"
#include "klib/threading/kmutex.h"
#include "balrog_os/file_system/gpt/gpt_struct.h"

typedef struct _fs_device_t
{
    char* name;
    char* path;
    kmutex_t lock;
    uint32_t unique_id;
    uint8_t type;
    uint64_t first_lba;
    gpt_header_t *gpt_header;
    gpt_partition_t *gpt_partition;
    uint8_t* partition_table;
    int (*read)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    int (*write)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    struct _file_system_t* fs;
    void* drive;
} __attribute__((packed)) fs_device_t;

uint64_t get_first_lba(fs_device_t* device);

void fs_device_init(fs_device_t* device);

void fs_add_device(fs_device_t* device);
