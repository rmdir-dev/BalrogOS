#include "balrog_os/tasking/tasking.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/tasking/proc_sleep.h"
#include "balrog_os/memory/kstack.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/gdt/gdt.h"
#include "balrog_os/cpu/rflags/rflag.h"
#include "balrog_os/tasking/elf/elf.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog/memory/proc_mem.h"
#include "balrog_os/user/user_manager.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
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
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : initializing process %s", name);
    process* proc = vmalloc(sizeof(process));
    memset(proc, 0, sizeof(process));
    proc->name = name;
    proc->pid = ++next_pid;
    proc->state = PROCESS_STATE_READY;
    proc->PML4T = pmm_calloc();
    proc->exec = 0;
    proc->child = 0;
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : name : %s", name);
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : pid : %d", proc->pid);
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : state : %d", proc->state);
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : PML4T : 0%p", proc->PML4T);

    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : process initilized", name);
    return proc;
}

process* create_process(char* name, uintptr_t addr, uint8_t mode)
{
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : creating process %s", name);
    process* proc = new_process(name);
    proc->rip = mode == 3 ? PROCESS_TEXT : addr; 
    uintptr_t* virt = P2V(proc->PML4T); // Kernel space
    virt[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE; // to change process won't be able to write into kernel space
    uint32_t user = mode == 3 ? PAGE_USER : 0;
    proc->uid = 0;
    proc->gid = 0;
    proc->fd_size = 0;
    proc->cwd = NULL;
    proc->error_no = (void*) PROCESS_ERRNO;

    /*
    TEXT & DATA
    */
    void* phys;

    elf_header* header = addr;

    if(header->ei_mag == ELF_MAGIC)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "tasking : ELF magic detected");
        proc->rip = header->e_entry;
        elf_load_binary(header, addr, proc->PML4T, user | PAGE_PRESENT | PAGE_WRITE);
    } else
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "tasking : ELF magic not found");
        void* text = pmm_calloc();
        phys = V2P(addr);
        vmm_set_page(proc->PML4T, PROCESS_TEXT, text, user | PAGE_PRESENT);
        memcpy(P2V(text), addr, 4096);
    }

    /*
    HEAP
    */
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : creating process heap");
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_HEAP_START, phys, user | PAGE_PRESENT | PAGE_WRITE);
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : process heap created at : 0%p", phys);

    proc->brk = PROCESS_HEAP_START + PAGE_SIZE;
    /*
    STACK
    */
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : creating process stack");
    phys = pmm_calloc();
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x1000, phys, user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x2000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x3000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(proc->PML4T, PROCESS_STACK_TOP - 0x4000, pmm_calloc(), user | PAGE_PRESENT | PAGE_WRITE);
    phys = kstack_alloc();
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : process stack created at : 0%p", phys);

    /*
        SETUP THE STACK
    */
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : setting up process stack");
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

    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : process %s created", name);

    return proc;
}

int clean_process(process* proc, uint8_t clean_memory)
{
    kernel_debug_output(KDB_LVL_VERBOSE, "tasking : cleaning process %d", proc->pid);

    if(clean_memory)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "tasking : cleaning process memory");
        vmm_clean_page_table(proc->PML4T);
    }

    if(proc->cwd)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "tasking : cleaning process cwd");
        vmfree(proc->cwd);
    }

    if(proc->sleeper_node) {
        kernel_debug_output(KDB_LVL_VERBOSE, "tasking : cleaning process sleeper node");
        remove_sleeper(proc);
    }

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

static void copy_pages(page_table* src, page_table* dest, uint8_t level, uintptr_t vaddr, uint8_t copy_all, uint8_t ignore_stack)
{
    src = P2V(src);
    dest = P2V(dest);

    for(size_t i = 0; i < 512; i++)
    {
        // check if page exist and if it is not the kernel page (PML4T[511])
        if(src[i] != 0 && (i < 511 || level < 4))
        {
            uintptr_t caddr = vaddr;
            get_vaddr(i, level, &caddr);

            // if the page is not a page table (Either a PML4T, PDPT or PDT)
            if(level > 1)
            {
                // if there is already an assigned page do not allocate a new one
                // or if the page is the same as the parent page
                if(dest[i] == 0 || dest[i] == src[i]) {
                    dest[i] = pmm_calloc();
                }
                copy_pages(STRIP_FLAGS(src[i]), STRIP_FLAGS(dest[i]), level - 1, caddr, copy_all, ignore_stack);
                dest[i] |= PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
            }else 
            {
                if(caddr < PROCESS_STACK_BOT)
                {
                    if(copy_all) {
                        dest[i] = pmm_calloc();
                        uint8_t* source = P2V(STRIP_FLAGS(src[i]));
                        memcpy(P2V(dest[i]), source, 4096);
                        dest[i] |= PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
                    } else {
                        dest[i] = P2V(dest[i]);
                        dest[i] = STRIP_FLAGS(src[i]) | PAGE_PRESENT | PAGE_USER;
                    }
                } else if(!ignore_stack)
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

/**
 * @brief Duplicate the page table from src to dest
 *
 * @param src
 * @param dest
 * @param level
 * @param vaddr
 */
static void duplicate_pages(page_table* src, page_table* dest, uint8_t level, uintptr_t vaddr) {
    copy_pages(STRIP_FLAGS(src), STRIP_FLAGS(dest), level, vaddr, 1, 1);
}

/**
 * @brief Share the page table from src to dest
 *        Will only duplicate the stack.
 *
 * @param src
 * @param dest
 * @param level
 * @param vaddr
 */
static void share_pages(page_table* src, page_table* dest, uint8_t level, uintptr_t vaddr) {
    copy_pages(STRIP_FLAGS(src), STRIP_FLAGS(dest), level, vaddr, 0, 0);
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
    and that a page fault occur when attempting to write
    */
    process* new = new_process(proc->name);
    share_pages(proc->PML4T, new->PML4T, 4, 0);
    new->child = 1;
    new->forked_memory = 1;
    new->uid = proc->uid;
    new->gid = proc->gid;
    new->brk = proc->brk;
    if(proc->cwd) {
        new->cwd = vmalloc(strlen(proc->cwd) + 1);
        memcpy(new->cwd, proc->cwd, strlen(proc->cwd) + 1);
    }
    new->parent = proc;
    new->wait_size = 0;
    for (int i = 0; i < 5; ++i) {
        new->waiting[i] = 0;
    }

    // KERNEL STACK
    page_table* newkstack = P2V(new->PML4T);
    //Set kernel to new stack
    newkstack[511] = 0x2000 | PAGE_PRESENT | PAGE_WRITE;

    uintptr_t phys = kstack_alloc();
    new->kernel_stack_top = P2V(phys) + 4095;
//
    new->exec = 0;
    proc_insert_to_ready_queue(new);

    uint8_t* virt = ((uint8_t*) new->kernel_stack_top) - sizeof(task_register);

    new->rsp = virt;

    task_register* stack = virt;
    new->exec = 0;
    new->stack_top = regs->rbp;

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

    kernel_debug_output(KDB_LVL_INFO, "tasking : fork, pid %d -> pid %d, %s, rip 0%p rsp 0%p",
            proc->pid, new->pid, new->name, regs->rip, regs->rsp);

    return new->pid;
}

void copy_forked_process_memory(process* proc, uintptr_t copy_addrs)
{
//    uintptr_t* parent_page = vmm_get_page(proc->parent->PML4T, (void*)copy_addrs);
//    uintptr_t* child_page = pmm_alloc();
//    memcpy(P2V(child_page), P2V(parent_page), 4096);
//    vmm_set_page(proc->PML4T, (void*)copy_addrs, child_page, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    duplicate_pages(proc->parent->PML4T, proc->PML4T, 4, 0);
    proc->forked_memory = 0;
}

static int __copy_add_args_to_stack(process* proc, char** argv)
{
    void* phys = pmm_calloc();

    if(phys == 0) 
    {
        kernel_debug_output(KDB_LVL_ERROR, "tasking : no page for the argv of %s", proc->name);
        return -1;
    }

    // hold errno so must be writable
    vmm_set_page(proc->PML4T, PROCESS_START_DATA, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
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
            argc++;
            argv++;
        }
    }

    task_register* reg = ((uint8_t*) proc->kernel_stack_top) - sizeof(task_register);
    reg->rdi = argc;
    reg->rsi = PROCESS_START_DATA;

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

    if(fs_get_file(name, &file, &fd) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "tasking : exec %s, the file system does not have it", name);
    }

    kernel_debug_output(KDB_LVL_INFO, "tasking : exec %s, %d bytes at 0%p", name, file.size, file.data);

    process* proc = create_process(name, file.data, 3);

    if(current_running)
    {
        proc->uid = current_running->uid;
        proc->gid = current_running->gid;
        if(current_running->cwd) {
            proc->cwd = vmalloc(strlen(current_running->cwd) + 1);
            memcpy(proc->cwd, current_running->cwd, strlen(current_running->cwd) + 1);
        }

        if(!proc->cwd) {
            user_data* user = usm_get_user_data(proc->uid);
            proc->cwd = vmalloc(strlen(user->home) + 1);
            memcpy(proc->cwd, user->home, strlen(user->home) + 1);
        }

        if(strcmp(name, "/bin/auth") == 0)
        {
            proc->uid = 0;
            proc->gid = 0;
            if(proc->cwd) {
                vmfree(proc->cwd);
            }
            proc->cwd = NULL;
        }

        if(current_running->wait_size != 0)
        {
            for(size_t i = 0; i < 5; i++)
            {
                proc->waiting[i] = current_running->waiting[i];
                current_running->waiting[i] = 0;
            }
            proc->wait_size = current_running->wait_size;
            current_running->wait_size = 0;
        }

        // switch pids
        proc_transfert_to_waiting(current_running->pid);
        int pid = proc->pid;
        proc->pid = current_running->pid;
        current_running->pid = pid;
        current_running->state = PROCESS_STATE_WAITING;
        proc_insert_to_ready_queue(proc);
        //proc_insert_to_ready_queue(current_running);

        if(current_running->parent) {
            proc->parent = current_running->parent;
        }
    } else 
    {
        proc_insert_to_ready_queue(proc);
    }

    fs_close(&fd);

    if(__copy_add_args_to_stack(proc, argv) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "tasking : exec %s, the arguments could not be pushed", name);
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "tasking : %s is pid %d, entry 0%p", name, proc->pid, proc->rip);

    if(kill == 1)
    {
        proc_kill(current_running, 1);
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
        proc_to_sleep(current_running->pid, PROCESS_STATE_WAITING);
        return 0;
    }
    return -1;
}