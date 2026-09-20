#include "balrog_os/cpu/cpuid/cpuid.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/debug/debug_output.h"
#include <string.h>

process* current_running = NULL;

static cpu_info_t cpu_info = {};

static void __cpuid_copy(char* dest, const uint32_t* regs, size_t count)
{
    for(size_t i = 0; i < count; i++)
    {
        memcpy(dest + (i * 4), &regs[i], 4);
    }
}

static void __cpu_read_vendor()
{
    cpuid_regs_t regs;

    cpuid(CPUID_LEAF_VENDOR, 0, &regs);
    cpu_info.max_leaf = regs.eax;

    /*  ebx, edx, ecx ! not ebx ecx edx, or it reads GenuntelineI  */
    uint32_t vendor[3] = { regs.ebx, regs.edx, regs.ecx };
    __cpuid_copy(cpu_info.vendor, vendor, 3);
    cpu_info.vendor[12] = 0;
}

static void __cpu_read_features()
{
    cpuid_regs_t regs;

    if(cpu_info.max_leaf < CPUID_LEAF_FEATURES)
    {
        kernel_debug_output(KDB_LVL_WARNING, "cpu : cpuid stops at leaf 0%x, no feature leaf", cpu_info.max_leaf);
        return;
    }

    cpuid(CPUID_LEAF_FEATURES, 0, &regs);

    /* family and model are split : the extended halves only count past 0xF,
       see intel sdm vol 2A, CPUID, table 3-8. */
    uint8_t family = (regs.eax >> 8) & 0xF;
    uint8_t model = (regs.eax >> 4) & 0xF;

    if(family == 0xF)
    {
        family += (regs.eax >> 20) & 0xFF;
    }

    if(family == 0x6 || family == 0xF)
    {
        model += ((regs.eax >> 16) & 0xF) << 4;
    }

    cpu_info.family = family;
    cpu_info.model = model;
    cpu_info.stepping = regs.eax & 0xF;

    cpu_info.logical_count = (regs.ebx >> 16) & 0xFF;
    cpu_info.initial_apic_id = (regs.ebx >> 24) & 0xFF;

    cpu_info.features_ecx = regs.ecx;
    cpu_info.features_edx = regs.edx;
}

static void __cpu_read_brand()
{
    cpuid_regs_t regs;

    cpuid(CPUID_LEAF_EXT_MAX, 0, &regs);
    cpu_info.max_ext_leaf = regs.eax;

    if(cpu_info.max_ext_leaf < CPUID_LEAF_BRAND_3)
    {
        /* no brand string, the vendor is all we get */
        memcpy(cpu_info.brand, cpu_info.vendor, 13);
        return;
    }

    for(uint32_t i = 0; i < 3; i++)
    {
        cpuid(CPUID_LEAF_BRAND_1 + i, 0, &regs);

        uint32_t brand[4] = { regs.eax, regs.ebx, regs.ecx, regs.edx };
        __cpuid_copy(cpu_info.brand + (i * 16), brand, 4);
    }

    cpu_info.brand[48] = 0;
}

const cpu_info_t* cpu_get_info()
{
    return &cpu_info;
}

int init_cpu_info()
{
    __cpu_read_vendor();
    __cpu_read_features();
    __cpu_read_brand();

    KERNEL_LOG_INFO("cpu : %s", cpu_info.brand);
    KERNEL_LOG_INFO("cpu : %s family %d model %d stepping %d, %d logical, apic id %d",
            cpu_info.vendor, cpu_info.family, cpu_info.model, cpu_info.stepping,
            cpu_info.logical_count, cpu_info.initial_apic_id);

    kernel_debug_output(KDB_LVL_INFO, "cpu : leaf max 0%x, extended max 0%x, ecx 0%x edx 0%x",
            cpu_info.max_leaf, cpu_info.max_ext_leaf, cpu_info.features_ecx, cpu_info.features_edx);


    if(!(cpu_info.features_edx & CPUID_EDX_APIC))
    {
        kernel_debug_output(KDB_LVL_ERROR, "cpu : no local apic, the timer will fall back on the pit");
    }

    if(!(cpu_info.features_edx & CPUID_EDX_PGE))
    {
        kernel_debug_output(KDB_LVL_ERROR, "cpu : no PGE, CR4.PGE is set anyway and the global bit does nothing");
    }

    if(!(cpu_info.features_edx & CPUID_EDX_MSR))
    {
        kernel_debug_output(KDB_LVL_ERROR, "cpu : no msr, IA32_GS_BASE will not work for the per cpu block");
    }

    return 0;
}