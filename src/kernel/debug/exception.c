#include "balrog_os/debug/exception.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/tasking/process.h"
#include "balrog/terminal/term.h"
#include "balrog_os/cpu/cr/control_register.h"

extern process* current_running;

static void __dump_context(interrupt_regs* stack_frame)
{
    kernel_debug_output(KDB_LVL_CRITICAL, "  rip 0%x  cs 0%x  rflags 0%x",
                        stack_frame->rip, stack_frame->cs, stack_frame->rflags);
    kernel_debug_output(KDB_LVL_CRITICAL, "  rsp 0%x  ss 0%x  cr3    0%x",
                        stack_frame->rsp, stack_frame->ss, read_cr3());
    kernel_debug_output(KDB_LVL_CRITICAL, "  rax 0%x  rbx 0%x  rcx 0%x  rdx 0%x",
                        stack_frame->rax, stack_frame->rbx, stack_frame->rcx, stack_frame->rdx);
    kernel_debug_output(KDB_LVL_CRITICAL, "  rsi 0%x  rdi 0%x  rbp 0%x",
                        stack_frame->rsi, stack_frame->rdi, stack_frame->rbp);
    kernel_debug_output(KDB_LVL_CRITICAL, "  r8  0%x  r9  0%x  r10 0%x  r11 0%x",
                        stack_frame->r8, stack_frame->r9, stack_frame->r10, stack_frame->r11);
    kernel_debug_output(KDB_LVL_CRITICAL, "  r12 0%x  r13 0%x  r14 0%x  r15 0%x",
                        stack_frame->r12, stack_frame->r13, stack_frame->r14, stack_frame->r15);

    // retrieve kernelspace/userspace through cs
    kernel_debug_output(KDB_LVL_CRITICAL, "  faulted in ring %d", (uint32_t)(stack_frame->cs & 0x3));
}

static void __halt_forever()
{
    while(1)
    {
        asm volatile("hlt");
    }
}

static interrupt_regs* __general_protection_fault_handler(interrupt_regs* stack_frame)
{
    kernel_debug_output(KDB_LVL_CRITICAL, "=== GENERAL PROTECTION FAULT ===");

    uint64_t error = stack_frame->error_code;

    kernel_debug_output_no_ln(KDB_LVL_CRITICAL, "");
    // Doc : https://wiki.osdev.org/Exceptions#General_Protection_Fault
    // Error code: The General Protection Fault sets an error code, which is the segment selector index
    // when the exception is segment related. Otherwise, 0.
    if(error == 0)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "  cause not segment related");
        kernel_debug_output(KDB_LVL_CRITICAL, "  privilege instruction at CPL != 0,");
        kernel_debug_output(KDB_LVL_CRITICAL, "  non canonical address, or reserved bit written.");
    } else
    {
        /**
         * https://wiki.osdev.org/Exceptions#Selector_Error_Code
         *
         *  31         16   15         3   2   1   0
         * +---+--  --+---+---+--  --+---+---+---+---+
         * |   Reserved   |    Index     |  Tbl  | E |
         * +---+--  --+---+---+--  --+---+---+---+---+
         */

        static const char* tbl[4] = {
            // Tbl	2 bits	IDT/GDT/LDT table	This is one of the following values:
            "GDT",  // 0b00	The Selector Index references a descriptor in the GDT.
            "IDT",  // 0b01	The Selector Index references a descriptor in the IDT.
            "LDT",  // 0b10	The Selector Index references a descriptor in the LDT.
            "IDT"   // 0b11	The Selector Index references a descriptor in the IDT.
        };
        kernel_debug_output(KDB_LVL_CRITICAL, "  selector %s[%d]%s",
                tbl[(error >> 1) & 0x3], // Tbl 2 bits IDT/GDT/LDT table (values above)
                (uint32_t) ((error >> 3) & 0x1FFF), // Index 13 bits Selector Index	The index in the GDT, IDT or LDT.
                (error & 0x01) ? " (external event)" : "" // E 1 bit External When set, the exception originated externally to the processor.
            );
    }

    __dump_context(stack_frame);
    kernel_debug_output(KDB_LVL_CRITICAL, "===        END  FAULT        ===");
    __halt_forever();
    return stack_frame;
}

static interrupt_regs* __page_fault_handler(interrupt_regs* stack_frame)
{
    // get the value of the CR2 register
    // as the address register contain the address that cause the page fault
    uintptr_t address = read_cr2();
    uint64_t error = stack_frame->error_code;

    // protection fault in user mode on forked process
    // Currently lazy fork TODO : COW (Copy On Write) fork.
    if(current_running && (stack_frame->error_code & 0x04 || stack_frame->error_code & 0x01) && current_running->forked_memory) {
        kernel_debug_output(KDB_LVL_INFO, "forked process memory protection fault -> copy parent memory \n");
        uintptr_t fault_base_address = address & 0xfffffffffffff000;
        copy_forked_process_memory(current_running, fault_base_address);
        return stack_frame;
    }

    kernel_debug_output(KDB_LVL_CRITICAL, "===        PAGE FAULT        ===");

    /**
     * https://wiki.osdev.org/Exceptions#Page_Fault
     *
     *   8    6    5    4    3    2    1    0
     *  +----+----+----+----+----+----+----+----+
     *  |SGX | SS | PK | ID | RSV| US | WR |  P |
     *  +----+----+----+----+----+----+----+----+
     *
     *      Length	Name	Description
     *  P	1 bit	Present	When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
     *  W	1 bit	Write	When set, the page fault was caused by a write access. When not set, it was caused by a read access.
     *  U	1 bit	User	When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
     *  R	1 bit	Reserved write	When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
     *  Note that this includes invalid physical addresses (e.g. 1<<40 on a processor with only 39 physical bits).
     *
     *  I	1 bit	Instruction Fetch	When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.
     *  PK	1 bit	Protection key	When set, the page fault was caused by a protection-key violation. The PKRU register (for user-mode accesses) or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.
     *  SS	1 bit	Shadow stack	When set, the page fault was caused by a shadow stack access.
     *  SGX	1 bit	Software Guard Extensions	When set, the fault was due to an SGX violation. The fault is unrelated to ordinary paging.
     */
    kernel_debug_output(KDB_LVL_CRITICAL, "  cause %s | %s | %s%s%s",
            // P	1 bit	Present	When set, the page fault was caused by a page-protection violation.
            //              When not set, it was caused by a non-present page.
            (error & 0x01) ? "protection violation" : "page not present",
            // W	1 bit	Write	When set, the page fault was caused by a write access.
            //              When not set, it was caused by a read access.
            (error & 0x02) ? "write" : "read",
            // U	1 bit	User	When set, the page fault was caused while CPL = 3. This does not necessarily mean
            //                      that the page fault was a privilege violation.
            (error & 0x04) ? "user" : "kernel",
            // R	1 bit	Reserved write	When set, one or more page directory entries contain reserved bits which are set to 1.
            //              This only applies when the PSE or PAE flags in CR4 are set to 1.
            //              Note that this includes invalid physical addresses (e.g. 1<<40 on a processor with only 39 physical bits).
            (error & 0x08) ? "| reserved bit set" : "",
            // I	1 bit	Instruction Fetch	When set, the page fault was caused by an instruction fetch.
            //              This only applies when the No-Execute bit is supported and enabled.
            (error & 0x10) ? " | instruction fetch" : ""
        );

    /**
     * Manage PK, SS, SGX
     *
     * They are not enabled yet, so this should not arrive, but they're already there.
     */
    if (error & 0x8060)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "  ANOMALY %s %s %s",
                // PK	1 bit	Protection key	When set, the page fault was caused by a protection-key violation.
                //              The PKRU register (for user-mode accesses)
                //              or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.
                (error & 0x0020) ? "protection key" : "",
                // SS	1 bit	Shadow stack	When set, the page fault was caused by a shadow stack access.
                (error & 0x0040) ? "shadow stack" : "",
                // SGX	1 bit	Software Guard Extensions	When set, the fault was due to an SGX violation.
                //              The fault is unrelated to ordinary paging.
                (error & 0x8000) ? "SGX" : ""
            );
    }

    __dump_context(stack_frame);
    kernel_debug_output(KDB_LVL_CRITICAL, "===        END  FAULT        ===");
    __halt_forever();
    
    return stack_frame;
}

static interrupt_regs* __invalid_opcode_handler(interrupt_regs* stack_frame)
{
    // Most of the time, either a jump into data through a corrupted stack or overwritten function pointer.
    kernel_debug_output(KDB_LVL_CRITICAL, "===      INVALID OPCODE      ===");
    __dump_context(stack_frame);
    kernel_debug_output(KDB_LVL_CRITICAL, "===        END  FAULT        ===");
    __halt_forever();

    return stack_frame;
}

static interrupt_regs* __double_fault_handler(interrupt_regs* stack_frame)
{
    /**
     * Doc : https://wiki.osdev.org/Exceptions#Double_Fault
     *
     * runs on its own IST stack, otherwise a stack overflow would fault
     * again here and turn into a triple fault : a silent reboot.
     * the error code is always zero, and the state is never recoverable.
     */
    kernel_debug_output(KDB_LVL_CRITICAL, "===       DOUBLE  FAULT      ===");

    __dump_context(stack_frame);
    kernel_debug_output(KDB_LVL_CRITICAL, "===        END  FAULT        ===");

    __halt_forever();
    return stack_frame;
}

int init_exception()
{
    register_interrupt_handler(INT_GP, &__general_protection_fault_handler);
    register_interrupt_handler(INT_PF, &__page_fault_handler);
    register_interrupt_handler(INT_UD, &__invalid_opcode_handler);
    register_interrupt_handler(INT_DF, &__double_fault_handler);

    return 0;
}