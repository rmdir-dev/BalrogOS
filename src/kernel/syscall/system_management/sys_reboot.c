#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/acpi/acpi.h"
#include "balrog/debug/debug.h"
#include "balrog/system/reboot.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/debug/klog.h"

extern void sys_reboot(interrupt_regs* stack_frame)
{
    if (stack_frame->rdi == BALROG_REBOOT_MAGIC1 && stack_frame->rsi == BALROG_REBOOT_MAGIC2)
    {
        switch (stack_frame->rdx)
        {
        case BALROG_REBOOT_POWER_OFF:
            kernel_debug_output(KDB_LVL_INFO, "sys_reboot() : Powering off the system on");
            // force flush
            klog_force_flush_buffers();
            acpi_power_off();
            break;
        case BALROG_REBOOT_RESTART:
            kernel_debug_output(KDB_LVL_INFO, "sys_reboot() : Rebooting system on");
            // force flush
            klog_force_flush_buffers();
            acpi_reboot();
            break;
        default:
            kernel_debug_output(KDB_LVL_ERROR, "sys_reboot() : wrong reboot command : 0%x", stack_frame->rdx);
            break;
        }
        return;
    }

    kernel_debug_output(KDB_LVL_ERROR, "sys_reboot() : wrong magic numbers !");
}
