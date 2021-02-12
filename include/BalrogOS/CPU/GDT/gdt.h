#pragma once
#include <stdint.h>

/* Defines a GDT entry. We say packed, because it prevents the
*  compiler from doing things that it thinks is best: Prevent
*  compiler "optimization" by packing */
typedef struct __gdt_entry
{
    /* Limits */
	unsigned short limit_low;
	/* Segment address */
	unsigned short base_low;
	unsigned char base_middle;
	/* Access modes */
	unsigned char access;
	unsigned char granularity;
	unsigned char base_high;
} __attribute__((packed)) gdt_entry;

typedef struct __gdt_ptr
{
	unsigned short limit;
	uint64_t base;
} __attribute__((packed)) gdt_ptr;

#define SEG_KCODE 0x08
#define SEG_KDATA 0x10
#define SEG_UDATA 0x18
#define SEG_UCODE 0x20
#define   SEG_TSS        0x28

#define   GDT_WRITE      (1LL<<41)
#define   GDT_EXECUTE    (1LL<<43)
#define   GDT_CODEDATA   (1LL<<44)
#define   GDT_PRESENT    (1LL<<47)
#define   GDT_64BIT      (1LL<<53)
#define   GDT_RING3      (3LL<<45)
#define   GDT_TSS        (9LL<<40)

#define   TSS_BLO(base)  ((((base)&0xFFFF)<<16) | ((((base)>>16)&0xFF)<<32) | ((((base)>>24)&0xFF)<<56))
#define   TSS_BHI(base)  (((base)>>32)&0xFFFFFFFF)
#define   TSS_LIM(lim)   (((lim)&0xFFFF) | ((((lim)>>16)&0xF)<<48))

void init_gdt();