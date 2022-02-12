#pragma once

// maximum number of opened file
#define FS_MAX_FILE     100

// FS buffer size = 32MiB
#define FS_BUFFER_SIZE (32 * (1024 * 1024))

#define FS_CACHE_OFFSET 0xffffffa000000000