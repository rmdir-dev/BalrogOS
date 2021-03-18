#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/Memory/kstack.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/CPU/RFLAGS/rflag.h"
#include "BalrogOS/Tasking/elf/elf.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include <string.h>

#define PROCESS_STACK_TOP   0x00007ffd0e212000
#define PROCESS_STACK_BOT   0x00007ffd0e212000 - 0x800000 // stack top - 8MiB
#define PROCESS_START_DATA  0x00007ffd0e212000
#define PROCESS_HEAP_BOTTOM 0x000055c0603d3000
#define PROCESS_TEXT        0x0000000000400000

uint64_t next_pid = 0;

typedef struct task_register_t
{
    //////////////////////////////////////////////////////////
    //                  STACK TOP
    //////////////////////////////////////////////////////////
    uint64_t rbp;           //
    uint64_t r15;           //
    uint64_t r14;           //
    uint64_t r13;           //
    uint64_t r12;           //
    uint64_t r11;           //
    uint64_t r10;           //
    uint64_t r9;            //
    uint64_t r8;            //
    uint64_t rdi;           //
    uint64_t rsi;           //
    uint64_t rdx;           //
    uint64_t rcx;           //
    uint64_t rbx;           //
    uint64_t rax;           // ALl the registers to handle ISR

    //////////////////////////////////////////////////////////
    //                  STACK BOTTOM
    //////////////////////////////////////////////////////////
    uint64_t rip;       // rip = next instruction
    uint64_t cs;        // code segment
    uint64_t rflags;    // rflags
    uint64_t rsp;       // stack pointer
    uint64_t ss;        // Stack segment
    //////////////////////////////////////////////////////////
    //                  STACK FRANE END
    //////////////////////////////////////////////////////////
} __attribute__((packed)) task_register;

process* new_process(char* name)
{
    process* proc = vmalloc(sizeof(process));
    memset(proc, 0, sizeof(process));
    proc->name = name;
    proc->pid = ++next_pid;
    proc->state = PROCESS_STATE_READY;
    proc->PML4T = pmm_calloc();
    proc->exec = 0;
    return proc;
}

process* create_process(char* name, uintptr_t addr, uint8_t mode)
{
    process* proc = new_process(name);
    proc->rip = mode == 3 ? PROCESS_TEXT : addr; 
    uintptr_t* virt = PHYSICAL_TO_VIRTUAL(proc->PML4T); // Kernel space
    virt[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE; // to change process won't be able to write into kernel space
    uint32_t user = mode == 3 ? PAGE_USER : 0; 

    /*
    TEXT & DATA
    */
    uintptr_t phys;

    elf_header* header = addr;

    if(header->ei_mag == ELF_MAGIC)
    {
        proc->rip = header->e_entry;
        elf_load_binary(header, addr, proc->PML4T, user | PAGE_PRESENT | PAGE_WRITE);
    } else 
    {
        uintptr_t text = pmm_calloc();
        phys = VIRTUAL_TO_PHYSICAL(addr);
        vmm_set_page(proc->PML4T, PROCESS_TEXT, text, user | PAGE_PRESENT);
        memcpy(PHYSICAL_TO_VIRTUAL(text), addr, 4096);
    }

    /*
    HEAP
    */
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_HEAP_BOTTOM, phys, user | PAGE_PRESENT | PAGE_WRITE);

    /*
    STACK
    */
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x1000, phys, user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x2000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x3000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x4000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    phys = kstack_alloc();

    /*
        SETUP THE STACK
    */
    proc->stack_top = PROCESS_STACK_TOP - 1;

    proc->kernel_stack_top = PHYSICAL_TO_VIRTUAL(phys) + 4095;
    virt = ((uint8_t*) proc->kernel_stack_top) - sizeof(task_register);


    proc->rsp = virt;// PROCESS_STACK_TOP - sizeof(task_register) - 1;
    
    task_register* stack = virt;

    /*
    Don't set IOPL to 3
    IOPL 3 mean that RING 3 will be able to use cli and other kind of restricted instruction.
    */
    stack->rflags = RFLAG_IF;

    if(mode == 3)
    {
        stack->ss = SEG_UDATA | 3;
        stack->rsp = proc->stack_top;
        stack->cs = SEG_UCODE | 3;
    } else
    {
        stack->ss = SEG_KDATA;
        stack->rsp = proc->stack_top;
        stack->cs = SEG_KCODE;
    }

    stack->rip = proc->rip;
    stack->r15 = 0;
    stack->r14 = 0;
    stack->r13 = 0;
    stack->r12 = 0;
    stack->r11 = 0;
    stack->r10 = 0;
    stack->r9 = 0;
    stack->r8 = 0;
    stack->rbp = proc->stack_top;
    stack->rdi = 0;
    stack->rsi = 0;
    stack->rdx = 0;
    stack->rcx = 0;
    stack->rbx = 0;
    stack->rax = 0;

    return proc;
}

int clean_process(process* proc)
{
    vmm_clean_page_table(proc->PML4T);
    vmfree(proc);
    return 0;
}

void copy_pages(page_table* src, page_table* dest, uint8_t level)
{
    
}

extern process* current_running;

int fork_process(process* proc)
{
    /*
    1 Create a new PML4T                : X
    2 Copy the page table in read only  : X
    3 Add process to process tree       : V
    4 Add process to ready queue.       : V

    COPY the page table only when a page is accessed
    and that a page fault occure when attempting to write
    */
    process* new = new_process(proc->name);

    return 0;
}

static int _copy_add_args_to_stack(process* proc, char** argv)
{
    uint8_t* phys = pmm_calloc();

    vmm_set_page(proc->PML4T, PROCESS_START_DATA, phys, PAGE_PRESENT | PAGE_USER);
    uint64_t* array = PHYSICAL_TO_VIRTUAL(phys);
    char* data = PHYSICAL_TO_VIRTUAL(phys) + 0x100;
    
    int argc = 0;

    if(argv != 0)
    {
        while(*argv)
        {
            strcpy(data, *argv);
            size_t len = strlen(*argv);
            data[len] = 0;
            array[argc] = PROCESS_START_DATA | ((uint64_t)data) % 0x1000;
            data += len + 1;
            //kprint("argv : 0%p | len : %d | %s\n", array[argc], len, *argv);
            argc++;
            argv++;
        }
    }

    task_register* reg = ((uint8_t*) proc->kernel_stack_top) - sizeof(task_register);
    //kprint("count : %d \n", argc);
    reg->rdi = argc;
    reg->rsi = PROCESS_START_DATA;

    if(phys == 0) 
    {
        return -1;
    }
    return 0;
}

int exec_process(const char* name, char** argv, uint8_t kill)
{
    /*
    1 Load the binary file          : V
    2 Create a new process          : V
    3 Add process to process tree   : V
    4 Add process to ready queue    : V
    5 Set the process stack         : V
        argc = RDI
        argv = RSI
    6 Kill the current process      : V

    PROCESS STACK :
        meta data top :
            0x00007ffd0e213000
            contain argv array and the args themselves.

        active stack top : 
            0x00007ffd0e212000
    */
    fs_fd fd;
    fs_file file;
    fs_get_file(name, &file, &fd);
    process* proc = create_process(name, file.data, 3);
    proc_insert_to_ready_queue(proc);
    fs_close(&fd);
    
    if(_copy_add_args_to_stack(proc, argv) != 0)
    {
        return -1;
    }

    if(kill == 1)
    {
        proc_kill_process(current_running->pid);
    }

    return 0;
}

int wait_process(int pid_to_wait)
{
    /*
    1 add process to waiting proc list  : V
        waiting proc list should use the proc_to_wait pid as key
        and have an array of X pid containing the pids of waiting process.
    2 Swtich process to waiting state.  : V
    */
    proc_add_to_waiting(current_running->pid, pid_to_wait);
    proc_transfert_to_waiting(current_running->pid);
    return 0;
}