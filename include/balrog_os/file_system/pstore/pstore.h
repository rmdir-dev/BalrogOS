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

#define PSTORE_RAM_SIZE             0x200000UL

/*  ram_core.c : sig, start and size, three 32 bit words, then the data. an
    atomic_t is an int, so there is no padding to work around.  */
#define PSTORE_RAM_HDR_SIZE         12
#define PSTORE_RAM_DATA_SIZE        (PSTORE_RAM_SIZE - PSTORE_RAM_HDR_SIZE)

#define PSTORE_RAM_SIG              0x43474244U

#define PSTORE_MAX_BACKEND          4

/*
based on linux's pstore struct
*/
typedef struct __pstore_record_t
{
    uint8_t type;               // PSTORE_TYPE_DMESG, the only front end here
    uint8_t reason;             // PSTORE_REASON_*, what brought us here
    uint8_t compressed;         // always 0, there is no deflate in this kernel
    uint32_t count;             // records of this reason since boot

    uint64_t time_sec;
    uint32_t time_nsec;

    const char* part[2];       // the log, oldest first
    size_t part_size[2];       // part_size[1] is 0 while the ring has not lapped
} pstore_record_t;

typedef struct __pstore_backend_t
{
    const char* name;
    int (*write)(const pstore_record_t* record);
} pstore_backend_t;

typedef struct __pstore_ram_zone_t
{
    uint32_t sig;               // PSTORE_RAM_SIG, or linux does not look further
    uint32_t start;             // write head, an offset into data
    uint32_t size;              // how many bytes of data are valid
    uint8_t  data[];            // the ==== line, then the log
} __attribute__((packed)) pstore_info_t;

pstore_info_t* get_pstore_info();

int init_pstore();