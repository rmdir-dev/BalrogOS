#include "balrog_os/drivers/keyboard/keyboard.h"
#include "balrog_os/drivers/screen/vga_driver.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/syscall/syscall_guard.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"
#include "klib/io/kprint.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

extern process* current_running;

// %rax     System call             %rdi                %rsi                        %rdx                %r10                    %r8             %r9
// 165	    sys_mount	            char *dev_name	    char *dir_name	        char *type	        unsigned long flags	void *data
int sys_mount(interrupt_regs* stack_frame)
{
    const char* dev_name = stack_frame->rdi;
    const char* dir_name = stack_frame->rsi;

    return fs_mount(dir_name, dev_name);
}


int sys_umount(interrupt_regs* stack_frame)
{
    return 0;
}