#pragma once

#include <stdint.h>

struct _file_system;

typedef enum _fs_format
{
    _EXT2_FORMAT = 0
}fs_format;

typedef struct _fs_device
{
    char* name;
    uint32_t unique_id;
    uint8_t type;
    uint8_t (*read)(struct _fs_device* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    uint8_t (*write)(struct _fs_device* device, uint8_t* buffer, uint64_t lba, uint64_t len);
    struct _file_system* fs;
} __attribute__((packed)) fs_device;

typedef struct _file_system
{
    char* name;
    int (*probe)(fs_device* device);
    int (*open)(fs_device* dev, const char* filename, uint8_t* buffer);
    int (*write)(fs_device* dev, const char* filename, uint8_t* buffer, uint64_t offset, uint64_t len);
    int (*touch)(fs_device* dev, const char* filename);
    int (*list)(fs_device* dev, const char* dirname, uint8_t* buffer);
    int (*mkdir)(fs_device* dev, const char* dirname);
    void* fs_data;
} __attribute__((packed)) file_system;

void init_file_system();