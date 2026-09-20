#include "balrog_os/cpu/tsc/tsc.h"
#include "balrog_os/cpu/cpuid/cpuid.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/debug/debug_output.h"

/*
Time Stamp Counter
Documentation : 
    TSC : https://wiki.osdev.org/TSC
    PIT : https://wiki.osdev.org/Programmable_Interval_Timer
*/

/*
How many tsc cycles go by in 100ns. 0 until we have measured it, and we only
measure it once.
*/
static uint64_t cycles_per_100ns = 0;

uint64_t tsc_read()
{
    uint32_t low = 0;
    uint32_t high = 0;

    asm volatile("rdtsc" : "=a"(low), "=d"(high));

    return ((uint64_t) high << 32) | low;
}

/*
Documentation : https://wiki.osdev.org/Programmable_Interval_Timer
*/
static int __pit_poll_wait(uint16_t count)
{
    /*  gate on, speaker off. the bit 0 gates the counting, the bit 1 sends
        the output to the speaker and we don't want the noise.  */
    out_byte(0x61, (in_byte(0x61) & ~0x02) | 0x01);

    /*  10 110 000 : channel 2, lobyte then hibyte, mode 0, binary. the bits
        7-6 pick the channel, 0x30 would be the same byte for the channel 0
        and that one belongs to the scheduler.  */
    out_byte(0x43, 0xB0);
    out_byte(0x42, count & 0xFF);
    out_byte(0x42, count >> 8);

    /*  half a second of a very fast cpu, far more than the 54ms a full 16 bit
        count takes.  */
    uint64_t deadline = tsc_read() + 2500000000ull;

    /*  in mode 0 the output stays low until the count reaches 0, then it goes
        high and stays high.  */
    while(tsc_read() < deadline)
    {
        if(in_byte(0x61) & 0x20)
        {
            return 1;
        }
    }

    kernel_debug_output(KDB_LVL_ERROR, "pit : channel 2 never raised its output for a count of %d", count);
    return 0;
}

/*
What the cpu says about itself, in cycles per 100ns, the leaf 0x16 gives
the base frequency in megahertz. It returns 0 when the cpu does not carry the leaf.

Documentation : intel sdm, cpuid leaf 0x16
*/
static uint64_t __tsc_from_cpuid()
{
    cpuid_regs_t regs;
    uint32_t max_leaf = cpu_get_info()->max_leaf;

    if(max_leaf < CPUID_LEAF_TSC)
    {
        kernel_debug_output(KDB_LVL_INFO, "tsc : cpuid stops at leaf 0%x, no leaf 0x16 to ask", max_leaf);
        return 0;
    }

    cpuid(CPUID_LEAF_TSC, 0, &regs);

    kernel_debug_output(KDB_LVL_INFO, "tsc : cpuid leaf 0x16 says %d MHz base, %d MHz max, %d MHz bus",
            regs.eax, regs.ebx, regs.ecx);

    /*  eax is megahertz. a megahertz is a million cycles a second, and a
        second holds ten million times 100ns, so the ratio is just /10.  */
    return regs.eax / 10;
}

static void __tsc_calibrate()
{
    uint64_t start = tsc_read();
    int answered = __pit_poll_wait(11930);      // 11930 counts, so 10ms
    uint64_t elapsed = tsc_read() - start;

    /*  10ms is a hundred thousand times 100ns  */
    cycles_per_100ns = answered ? elapsed / 100000 : 0;

    /*  a cpu runs between 100 and 1000 cycles per 100ns. */
    if(cycles_per_100ns < 50 || cycles_per_100ns > 2000)
    {
        kernel_debug_output(KDB_LVL_INFO, "tsc : the pit gave %d cycles per 100ns, out of range, asking the cpu",
                cycles_per_100ns);
        cycles_per_100ns = __tsc_from_cpuid();
    } else
    {
        kernel_debug_output(KDB_LVL_INFO, "tsc : calibrated on the pit, %d cycles per 100ns", cycles_per_100ns);
    }

    if(cycles_per_100ns == 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "tsc : neither the pit nor the cpu answered, falling back on 1GHz");
        cycles_per_100ns = 100;                 // 1GHz, a plain guess
    }

    /*  every bounded wait in the kernel is measured against this number, so
        which of the three sources won is worth one line.  */
    kernel_debug_output(KDB_LVL_INFO, "tsc : %d cycles per 100ns, %d MHz", cycles_per_100ns, cycles_per_100ns * 10);
}

uint64_t tsc_per_100ns()
{
    if(cycles_per_100ns == 0)
    {
        __tsc_calibrate();
    }

    return cycles_per_100ns;
}

void tsc_wait_100ns(uint64_t units)
{
    uint64_t deadline = tsc_read() + units * tsc_per_100ns();

    while(tsc_read() < deadline)
    {
    }
}
