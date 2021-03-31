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
#include "balrog/memory/proc_mem.h"
#include <string.h>

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
    proc->child = 0;
    return proc;
}

process* create_process(char* name, uintptr_t addr, uint8_t mode)
{
    process* proc = new_process(name);
    proc->rip = mode == 3 ? PROCESS_TEXT : addr; 
    uintptr_t* virt = P2V(proc->PML4T); // Kernel space
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
        phys = V2P(addr);
        vmm_set_page(proc->PML4T, PROCESS_TEXT, text, user | PAGE_PRESENT);
        memcpy(P2V(text), addr, 4096);
    }

    /*
    HEAP
    */
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_HEAP_START, phys, user | PAGE_PRESENT | PAGE_WRITE);

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

    proc->kernel_stack_top = P2V(phys) + 4095;
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

static void get_vaddr(size_t index, uint8_t level, uintptr_t* vaddr)
{
    switch (level)
    {
    case 1:
        *vaddr |= PT_TO_VIRT(index);
        break;

    case 2:
        *vaddr |= PDT_TO_VIRT(index);
        break;

    case 3:
        *vaddr |= PDPT_TO_VIRT(index);
        break;

    case 4:
        *vaddr |= PML4T_TO_VIRT(index);
        break;
    
    default:
        break;
    }
}

static void copy_pages(page_table* src, page_table* dest, uint8_t level, uintptr_t vaddr)
{
    src = P2V(src);
    dest = P2V(dest);

    for(size_t i = 0; i < 512; i++)
    {
        if(src[i] != 0 && (i < 511 || level < 4))
        {
            uintptr_t caddr = vaddr;
            get_vaddr(i, level, &caddr);
            
            if(level > 1)
            {
                dest[i] = pmm_calloc();
                copy_pages(STRIP_FLAGS(src[i]), dest[i], level - 1, caddr);
                dest[i] |= PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
            }else 
            {
                if(caddr < PROCESS_STACK_BOT)
                {
                    dest[i] = P2V(dest[i]);
                    dest[i] = STRIP_FLAGS(src[i]) | PAGE_PRESENT | PAGE_USER;
                } else 
                {
                    dest[i] = pmm_calloc();
                    uint8_t* source = P2V(STRIP_FLAGS(src[i]));
                    memcpy(P2V(dest[i]), source, 4096);
                    dest[i] |= PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
                }
            }
        }
    }
}

extern process* current_running;

int fork_process(process* proc, interrupt_regs* regs)
{
    /*
    1 Create a new PML4T                : V
    2 Copy the page table in read only  : V
    3 Add process to process tree       : V
    4 Add process to ready queue.       : V

    TODO LATER
    COPY the page table only when a page is accessed
    and that a page fault occure when attempting to write
    */
    process* new = new_process(proc->name);
    copy_pages(proc->PML4T, new->PML4T, 4, 0);
    new->child = 1;

    // KERNEL STACK
    page_table* newkstack = P2V(new->PML4T);
    //Set kernel to new stack
    newkstack[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE;

    uintptr_t phys = kstack_alloc();
    new->kernel_stack_top = P2V(phys) + 4095;

    proc_insert_to_ready_queue(new);

    uint8_t* virt = ((uint8_t*) new->kernel_stack_top) - sizeof(task_register);

    new->rsp = virt;
    
    task_register* stack = virt;
    new->exec = 0;

    stack->rflags = RFLAG_IF;
    stack->ss = SEG_UDATA | 3;
    stack->rsp = regs->rsp;
    stack->cs = SEG_UCODE | 3;

    stack->rip = regs->rip;
    stack->r15 = regs->r15;
    stack->r14 = regs->r14;
    stack->r13 = regs->r13;
    stack->r12 = regs->r12;
    stack->r11 = regs->r11;
    stack->r10 = regs->r10;
    stack->r9 = regs->r9;
    stack->r8 = regs->r8;
    stack->rbp = regs->rbp;
    stack->rdi = regs->rdi;
    stack->rsi = regs->rsi;
    stack->rdx = regs->rdx;
    stack->rcx = regs->rcx;
    stack->rbx = regs->rbx;
    stack->rax = 0;

    return new->pid;
}

static int _copy_add_args_to_stack(process* proc, char** argv)
{
    uint8_t* phys = pmm_calloc();

    vmm_set_page(proc->PML4T, PROCESS_START_DATA, phys, PAGE_PRESENT | PAGE_USER);
    uint64_t* array = P2V(phys);
    char* data = P2V(phys) + 0x100;
    
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
    //kprint("count : %d | stack : 0%p \n", argc, reg);
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

    if(current_running)
    {
        if(current_running->wait_size != 0)
        {
            for(size_t i = 0; i < 5; i++)
            {
                proc->waiting[i] = current_running->waiting[i];
            }
            proc->wait_size = current_running->wait_size;
            current_running->wait_size = 0;
        }

        // switch pids
        proc_transfert_to_waiting(current_running->pid);
        int pid = proc->pid;
        proc->pid = current_running->pid;
        //kprint("pid : %d | 0%p \n", proc->pid, proc);
        current_running->pid = pid;
        current_running->state = PROCESS_STATE_WAITING;
        proc_insert_to_ready_queue(proc);
        //proc_insert_to_ready_queue(current_running);
    } else 
    {
        proc_insert_to_ready_queue(proc);
    }

    fs_close(&fd);

    if(_copy_add_args_to_stack(proc, argv) != 0)
    {
        return -1;
    }

    if(kill == 1)
    {
        proc_kill(current_running);
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
    if(proc_add_to_waiting(current_running->pid, pid_to_wait) == 0)
    {
        proc_to_sleep(current_running->pid);
        return 0;
    }
    return -1;
}