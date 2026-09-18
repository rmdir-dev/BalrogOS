#pragma once

#include "klib/data_structure/list.h"
#include "klib/threading/kmutex.h"

typedef struct _fs_device_t
{
    char* name;
    char* path;
    kmutex_t lock;
    uint32_t unique_id;
    uint8_t type;
    uint64_t part_lba_start;
    void (*read)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    void (*write)(struct _fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    struct _file_system_t* fs;
    void* drive;
} __attribute__((packed)) fs_device_t;

void fs_add_device(fs_device_t* device);
