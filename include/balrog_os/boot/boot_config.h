#pragma once

#include <stdint.h>

#define BOOT_CONFIG_PHYS        0x7700
#define BOOT_CONFIG_MAGIC       "# 1980TA"  // Durin's Bane wake up year
#define BOOT_CONFIG_MAGIC_LEN   8

#define BOOT_GUID_TEXT_LEN      37          // 36 char of the uuid + \0
#define BOOT_GUID_BYTE_LEN      16

typedef struct __boot_config_t
{
    char magic[BOOT_CONFIG_MAGIC_LEN];      // should be "# 1980TA"
    uint32_t reserved;                      // buffer

    uint8_t root_guid[BOOT_GUID_BYTE_LEN];
    uint8_t raw_debug_guid[BOOT_GUID_BYTE_LEN];
} __attribute__((__packed__)) boot_config_t;