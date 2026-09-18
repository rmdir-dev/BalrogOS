#pragma once

#include <stdint.h>

/**
 * Linux compatible pstore, use for debugging :
 * BalrogOS will write linux compatible pstore to store klogs & coredump.
 */

#define PSTORE_TYPE_DMESG           0
#define PSTORE_TYPE_MCE             1
#define PSTORE_TYPE_CONSOLE         2
#define PSTORE_TYPE_FTRACE          3

/* pstore physical address */
#define PSTORE_RAM_PHYS             0x7F000000UL
#define PSTORE_RAM_SIZE             0x200000UL

/* pstore virtual address */
#define PSTORE_RAM_VIRTUAL_BASE     0xFFFFFFFFA0000000

/*  ram_core.c : sig, start and size, three 32 bit words, then the data. an
    atomic_t is an int, so there is no padding to work around.  */
#define PSTORE_RAM_HDR_SIZE         12
#define PSTORE_RAM_DATA_SIZE        (PSTORE_RAM_SIZE - PSTORE_RAM_HDR_SIZE)

#define PSTORE_RAM_SIG              0x43474244U

typedef struct __pstore_ram_zone_t
{
    uint32_t sig;               // PSTORE_RAM_SIG, or linux does not look further
    uint32_t start;             // write head, an offset into data
    uint32_t size;              // how many bytes of data are valid
    uint8_t  data[];            // the ==== line, then the log
} __attribute__((packed)) pstore_ram_zone_t;

int init_pstore();