#pragma once 

#define EXT2_BLOCK_SIZE 4096
#define EXT2_BLOCK_SIZE_HINT 2
#define EXT2_FRAG_SIZE_HINT 2

#define EXT2_DEFAULT_DIR_ACCESS     0770    // rwxrwx---
#define EXT2_DEFAULT_FILE_ACCESS    0664    // rw-rw-r--

#define EXT2_ROOT_INODE             2       // the root directory is always inode 2

// number of block pointers held by one single indirect block
#define EXT2_SIBP_ENTRIES           (EXT2_BLOCK_SIZE / sizeof(uint32_t))

// 12 direct block pointers plus every single indirect one
#define EXT2_MAX_BLOCKS             (12 + EXT2_SIBP_ENTRIES)
