#include "balrog_os/cpu/apic/apic.h"
#include "balrog_os/cpu/cpuid/cpuid.h"
#include "balrog_os/cpu/tsc/tsc.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/debug/debug_output.h"

/*
Advanced Programmable Interrupt Controller
Documentation : 
    APIC : https://wiki.osdev.org/APIC#Local_APIC_registers
    APIC timer : https://wiki.osdev.org/APIC_timer
*/

#define LAPIC_ID            0x020
#define LAPIC_VERSION       0x030
#define LAPIC_EOI           0x0B0
#define LAPIC_SPURIOUS      0x0F0
#define LAPIC_LVT_TIMER     0x320
#define LAPIC_TIMER_INIT    0x380
#define LAPIC_TIMER_CURRENT 0x390
#define LAPIC_TIMER_DIVIDE  0x3E0

#define LAPIC_SOFTWARE_ENABLE   0x100       // bit 8 of the spurious register
#define LAPIC_TIMER_PERIODIC    0x20000     // bit 17 of the LVT entry
#define LAPIC_LVT_MASKED        0x10000     // bit 16 of any LVT entry

/*
0b0011 in the divide register is a divide by 16.
*/
#define LAPIC_DIVIDE_16 0x03

#define IA32_APIC_BASE          0x1B
#define APIC_BASE_ENABLE        0x800        // bit 11 of the msr

/*
Where the local apic registers are, once mapped. 0 when there is none (pit takes back control)
*/
static volatile uint32_t* lapic = 0;

static uint64_t __read_msr(uint32_t msr)
{
    uint32_t low = 0;
    uint32_t high = 0;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return ((uint64_t) high << 32) | low;
}

static void __write_msr(uint32_t msr, uint64_t value)
{
    asm volatile("wrmsr" : : "c"(msr), "a"((uint32_t) value),
        "d"((uint32_t)(value >> 32)));
}

static uint32_t __lapic_read(uint32_t reg)
{
    return lapic[reg / 4];
}

static void __lapic_write(uint32_t reg, uint32_t value)
{
    lapic[reg / 4] = value;
}

void lapic_eoi()
{
    if(lapic)
    {
        __lapic_write(LAPIC_EOI, 0);
    }
}

/*
The spurious vector has to point somewhere real as kernel_interrupt_handler spins
for ever on a vector nobody claimed. no eoi needed on this one.

Documentation : https://wiki.osdev.org/APIC#Spurious_Interrupt_Vector_Register
*/
static interrupt_regs* __spurious_handler(interrupt_regs* stack_frame)
{
    return stack_frame;
}

/*
Find the local apic and map it.
*/
static int __lapic_find()
{
    uint32_t features_edx = cpu_get_info()->features_edx;

    if(!(features_edx & CPUID_EDX_APIC))
    {
        kernel_debug_output(KDB_LVL_INFO, "lapic : cpuid leaf 1 edx 0%x has no local apic bit", features_edx);
        return -1;
    }

    uint64_t base = __read_msr(IA32_APIC_BASE);

    __write_msr(IA32_APIC_BASE, base | APIC_BASE_ENABLE);

    uintptr_t phys = base & 0xFFFFFF000;

    if(phys == 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "lapic : the apic base msr reads 0%x, no address to map", base);
        return -1;
    }

    /*  PAGE_NOCACHE because what is behind is the cpu's own register file and
        not memory. a cached read of a count that moves gives us the count it
        had the first time.  */
    if(!vmm_get_page(0, (void*) P2V(phys)))
    {
        vmm_set_page(0, (void*) P2V(phys), (void*) phys,
            PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE);
    }

    lapic = (volatile uint32_t*) P2V(phys);

    kernel_debug_output(KDB_LVL_INFO, "lapic : base msr 0%x, registers at 0%p, id 0%x version 0%x",
            base, (uintptr_t) lapic, __lapic_read(LAPIC_ID), __lapic_read(LAPIC_VERSION));

    return 0;
}

/*
How many times the timer counts down in a second.
*/
static uint32_t __lapic_timer_frequency()
{
    __lapic_write(LAPIC_TIMER_DIVIDE, LAPIC_DIVIDE_16);
    __lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    __lapic_write(LAPIC_TIMER_INIT, 0xFFFFFFFF);

    tsc_wait_100ns(100000);                     // 10ms

    uint32_t left = __lapic_read(LAPIC_TIMER_CURRENT);

    __lapic_write(LAPIC_TIMER_INIT, 0);

    /*  a left of 0xFFFFFFFF means it never moved, which is the whole point of
        measuring instead of trusting a constant.  */
    kernel_debug_output(KDB_LVL_INFO, "lapic timer : 0%x left of 0%x after 10ms, divider 16",
            left, 0xFFFFFFFF);

    /*  what it counted in a hundredth of a second  */
    return (0xFFFFFFFF - left) * 100;
}

int lapic_timer_init(uint32_t hz, pit_event event)
{
    if(__lapic_find() != 0)
    {
        kernel_debug_output(KDB_LVL_INFO, "lapic : not usable, the scheduler will need another timer");
        return -1;
    }

    register_interrupt_handler(INT_APIC_SPUR, &__spurious_handler);
    __lapic_write(LAPIC_SPURIOUS, LAPIC_SOFTWARE_ENABLE | INT_APIC_SPUR);

    uint32_t frequency = __lapic_timer_frequency();

    if(frequency < hz)
    {
        KERNEL_LOG_INFO("LAPIC timer : did not count, falling back on the pit");
        kernel_debug_output(KDB_LVL_ERROR, "lapic timer : %d counts a second is below the %d Hz asked", frequency, hz);
        return -1;
    }

    timer_set_event(event);
    register_interrupt_handler(INT_APIC_TIMER, &timer_lvt_handler);

    __lapic_write(LAPIC_TIMER_DIVIDE, LAPIC_DIVIDE_16);
    __lapic_write(LAPIC_LVT_TIMER, INT_APIC_TIMER | LAPIC_TIMER_PERIODIC);
    __lapic_write(LAPIC_TIMER_INIT, frequency / hz);

    KERNEL_LOG_INFO("LAPIC timer : 0%p, %d counts a second, %d Hz",
        (uintptr_t) lapic, frequency, hz);
    kernel_debug_output(KDB_LVL_INFO, "lapic timer : periodic on vector %d, initial count %d",
        INT_APIC_TIMER, frequency / hz);

    return 0;
}
