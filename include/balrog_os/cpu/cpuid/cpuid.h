#pragma once

#include <stdint.h>

/*
CPUID, what the cpu says about itself.
Documentation :
    OSDev : https://wiki.osdev.org/CPUID
    intel sdm vol 2A, CPUID instruction
    the vendor string comes back in ebx, edx, ecx in that order ! not ebx ecx edx.
*/
#define CPUID_LEAF_VENDOR       0x00000000  // max leaf in eax, vendor in ebx edx ecx
#define CPUID_LEAF_FEATURES     0x00000001  // family and model in eax, flags in ecx edx
#define CPUID_LEAF_TSC          0x00000016  // base, max and bus frequency in MHz
#define CPUID_LEAF_EXT_MAX      0x80000000  // max extended leaf
#define CPUID_LEAF_BRAND_1      0x80000002  // the brand string, 16 bytes per leaf
#define CPUID_LEAF_BRAND_2      0x80000003
#define CPUID_LEAF_BRAND_3      0x80000004
#define CPUID_LEAF_TSC          0x00000016  // base, max and bus frequency in MHz

/*  leaf 1, edx  */
#define CPUID_EDX_FPU           (1 << 0)
#define CPUID_EDX_PSE           (1 << 3)    // 4MiB pages
#define CPUID_EDX_MSR           (1 << 5)
#define CPUID_EDX_PAE           (1 << 6)
#define CPUID_EDX_APIC          (1 << 9)
#define CPUID_EDX_PGE           (1 << 13)   // the global bit, we raise CR4.PGE
#define CPUID_EDX_PAT           (1 << 16)
#define CPUID_EDX_SSE           (1 << 25)
#define CPUID_EDX_SSE2          (1 << 26)
#define CPUID_EDX_HTT           (1 << 28)   // more than one logical cpu per package

/*  leaf 1, ecx  */
#define CPUID_ECX_SSE3          (1 << 0)
#define CPUID_ECX_SSSE3         (1 << 9)
#define CPUID_ECX_SSE41         (1 << 19)
#define CPUID_ECX_SSE42         (1 << 20)
#define CPUID_ECX_X2APIC        (1 << 21)
#define CPUID_ECX_XSAVE         (1 << 26)
#define CPUID_ECX_HYPERVISOR    (1 << 31)   // if virtual machine

typedef struct __cpu_info_t
{
    char vendor[13];            // who built it, 12 bytes zero eneded
    char brand[49];             // the marketing name, 48 bytes zero eneded
    uint32_t max_leaf;          // highest standard leaf cpuid answers
    uint32_t max_ext_leaf;      // highest 0x8000xxxx leaf cpuid answers
    uint8_t family;             // the generation, extended half folded in
    uint8_t model;              // the variant inside that family
    uint8_t stepping;           // silicon revision of that model
    uint8_t logical_count;      // what the package claims, HTT included
    uint32_t initial_apic_id;   // leaf 1 ebx bits 31:24
    uint32_t features_ecx;      // leaf 1 ecx flags, the CPUID_ECX_* bits
    uint32_t features_edx;      // leaf 1 edx flags, the CPUID_EDX_* bits
} cpu_info_t;

typedef struct __cpuid_regs_t
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_regs_t;

static inline __attribute__((always_inline)) void cpuid(uint32_t leaf, uint32_t sub, cpuid_regs_t* regs)
{
    asm volatile("cpuid"
            : "=a"(regs->eax), "=b"(regs->ebx), "=c"(regs->ecx), "=d"(regs->edx)
            : "a"(leaf), "c"(sub));
}

/**
 * @brief what the cpu answered at boot
 *
 * @return const cpu_info_t*
 */
const cpu_info_t* cpu_get_info();

int init_cpu_info();