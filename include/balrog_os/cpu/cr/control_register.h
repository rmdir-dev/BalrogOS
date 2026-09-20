#pragma once
#include <stdint.h>

/**
 * @brief Read the content of Control Register 0
 * The CR0 register is 32 bits long on the 386 and higher processors. On x64 processors in long mode,
 * it (and the other control registers) is 64 bits long. CR0 has various control flags that modify
 * the basic operation of the processor. Register CR0 is the 32 Bit version of the old Machine Status Word (MSW) register.
 * The MSW register was expanded to the Control Register with the appearance of the i386 processor.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @return Control Register 0 value
 */
static inline __attribute__((always_inline)) uintptr_t read_cr0()
{
    uintptr_t value;
    asm volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

/**
 * @brief Write into CR0 :
 *
 * Bit	Name	Full Name	Description
 * 0	PE	Protected Mode Enable	If 1, system is in protected mode, else, system is in real mode
 * 1	MP	Monitor co-processor	Controls interaction of WAIT/FWAIT instructions with TS flag in CR0
 * 2	EM	Emulation	If set, no x87 floating-point unit present, if clear, x87 FPU present
 * 3	TS	Task switched	Allows saving x87 task context upon a task switch only after x87 instruction used
 * 4	ET	Extension type	On the 386, it allowed to specify whether the external math coprocessor was an 80287 or 80387
 * 5	NE	Numeric error	On the 486 and later, enable internal x87 floating point error reporting when set, else enable PC-style error reporting from the internal floating-point unit using external logic[13]
 * 16	WP	Write protect	When set, the CPU cannot write to read-only pages when privilege level is 0
 * 18	AM	Alignment mask	Alignment check enabled if AM set, AC flag (in EFLAGS register) set, and privilege level is 3
 * 29	NW	Not-write through	Globally enables/disable write-through caching
 * 30	CD	Cache disable	Globally enables/disable the memory cache
 * 31	PG	Paging	If 1, enable paging and use the § CR3 register, else disable paging.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @param value
 */
static inline __attribute__((always_inline)) void write_cr0(uintptr_t value)
{
    asm volatile ("mov %0, %%cr0" :: "r"(value));
}

// CR1 Reserved, the CPU will throw a #UD exception when trying to access it.

/**
 * @brief Read the content of Control Register 2
 * Contains a value called Page Fault Linear Address (PFLA). When a page fault occurs,
 * the address the program attempted to access is stored in the CR2 register.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @return Control Register 2 value
 */
static inline __attribute__((always_inline)) uintptr_t read_cr2()
{
    uintptr_t value;
    asm volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

/**
 * CR2 should NOT be written manually :
 *
 * CR2 holds the Page Fault Linear Address (PFLA) and is written by the CPU itself
 * every time a page fault is raised.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 */

/**
 * @brief Read the content of Control Register 3
 * Used when virtual addressing is enabled, hence when the PG bit is set in CR0.
 * CR3 enables the processor to translate linear addresses into physical addresses by locating
 * the page directory and page tables for the current task. Typically, the upper 20 bits of CR3 become
 * the page directory base register (PDBR), which stores the physical address of the first page directory.
 * If the PCIDE bit in CR4 is set, the lowest 12 bits are used for the process-context identifier (PCID).
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @return Control Register 3 value
 */
static inline __attribute__((always_inline)) uintptr_t read_cr3()
{
    uintptr_t value;
    asm volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

/**
 * @brief Write into CR3 :
 *
 * Loads a new Page Directory Base Register (PDBR), which switches the address space the
 * processor translates through. This is what a context switch does when it moves from
 * one process to another.
 *
 * Bit	Name	Full Name	Description
 * 2-0	—N/a	(Reserved)	—N/a
 * 3	PWT	Page-level Write-Through	Controls the memory type used to access the first paging structure. Ignored when PCIDE is set in CR4.
 * 4	PCD	Page-level Cache Disable	Controls the memory type used to access the first paging structure. Ignored when PCIDE is set in CR4.
 * 11-5	—N/a	(Reserved)	—N/a
 * 63-12	PDBR	Page Directory Base Register	Physical address of the top level paging structure, the PML4 in 4-level paging.
 *
 * If PCIDE is set in CR4 the layout changes : bits 11:0 hold the process-context
 * identifier (PCID) instead of PWT and PCD.
 *
 * Writing CR3 flushes every non global TLB entry. Entries whose PTE has the global bit
 * set survive, as long as PGE is set in CR4. That side effect is the usual way to make
 * a page table change visible, and it is also why reloading CR3 is more expensive than
 * an INVLPG on a single page.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @param value
 */
static inline __attribute__((always_inline)) void write_cr3(uintptr_t value)
{
    asm volatile ("mov %0, %%cr3" :: "r"(value));
}

/**
 * @brief Read the content of Control Register 4
 * Used in protected mode to control operations such as virtual-8086 support, enabling I/O breakpoints,
 * page size extension and machine-check exceptions.
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @return Control Register 4 value
 */
static inline __attribute__((always_inline)) uintptr_t read_cr4()
{
    uintptr_t value;
    asm volatile("mov %%cr4, %0" : "=r"(value));
    return value;
}

/**
 * @brief Write into CR0 :
 *
 * Bit	Name	Full Name	Description
 * 0	VME	Virtual 8086 Mode Extensions	If set, enables support for the virtual interrupt flag (VIF) in virtual-8086 mode.
 * 1	PVI	Protected-mode Virtual Interrupts	If set, enables support for the virtual interrupt flag (VIF) in protected mode.
 * 2	TSD	Time Stamp Disable	If set, RDTSC instruction can only be executed when in ring 0, otherwise RDTSC can be used at any privilege level.
 * 3	DE	Debugging Extensions	If set, enables debug register based breaks on I/O space access.
 * 4	PSE	Page Size Extension	If set, enables 32-bit paging mode to use 4 MiB huge pages in addition to 4 KiB pages.
 *          If PAE is enabled or the processor is in x86-64 long mode this bit is ignored.[15]
 * 5	PAE	Physical Address Extension	If set, changes page table layout to translate 32-bit virtual addresses into extended 36-bit physical addresses.
 * 6	MCE	Machine Check Exception	If set, enables machine check interrupts to occur.
 * 7	PGE	Page Global Enabled	If set, address translations (PDE or PTE records) may be shared between address spaces.
 * 8	PCE	Performance-Monitoring Counter enable	If set, RDPMC can be executed at any privilege level, else RDPMC can only be used in ring 0.
 * 9	OSFXSR	Operating system support for FXSAVE and FXRSTOR instructions	If set, enables Streaming SIMD Extensions (SSE) instructions and fast FPU save & restore.
 * 10	OSXMMEXCPT	Operating System Support for Unmasked SIMD Floating-Point Exceptions	If set, enables unmasked SSE exceptions.
 * 11	UMIP	User-Mode Instruction Prevention	If set, the SGDT, SIDT, SLDT, SMSW and STR instructions cannot be executed if CPL > 0.[14]
 * 12	LA57	57-Bit Linear Addresses	If set, enables 5-Level Paging.[16][17]: 2–18 
 * 13	VMXE	Virtual Machine Extensions Enable	see Intel VT-x x86 virtualization.
 * 14	SMXE	Safer Mode Extensions Enable	see Trusted Execution Technology (TXT)
 * 15	[a]	(Reserved)	—N/a
 * 16	FSGSBASE	FSGSBASE Enable	If set, enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE.
 * 17	PCIDE	PCID Enable	If set, enables process-context identifiers (PCIDs).
 * 18	OSXSAVE	XSAVE and Processor Extended States Enable
 * 19	KL	Key Locker Enable	If set, enables the AES Key Locker instructions.
 * 20	SMEP[20]	Supervisor Mode Execution Protection Enable	If set, execution of code in a higher ring generates a fault.
 * 21	SMAP	Supervisor Mode Access Prevention Enable	If set, access of data in a higher ring generates a fault.[21]
 * 22	PKE	Protection Key Enable	See Intel 64 and IA-32 Architectures Software Developer's Manual.
 * 23	CET	Control-flow Enforcement Technology	If set, enables control-flow enforcement technology.[17]: 2–19 
 * 24	PKS	Enable Protection Keys for Supervisor-Mode Pages	If set, each supervisor-mode linear address is associated with a protection key when 4-level or 5-level paging is in use.[17]: 2–19 
 * 25	UINTR	User Interrupts Enable	If set, enables user-mode inter-processor interrupts and their associated instructions and data structures.
 * 26	—N/a	(Reserved)	—N/a
 * 27	LASS	Linear Address Space Separation
 * 28	LAM_SUP	Linear Address Masking for Supervisor pointers
 * 29	—N/a	(Reserved)	—N/a
 * 30	—N/a	(Reserved)	—N/a
 * 31	—N/a	(Reserved)	—N/a
 * 32	FRED	Flexible Return and Event Delivery
 * 63-33	—N/a	(Reserved)	—
 *
 * See full doc https://en.wikipedia.org/wiki/Control_register#Control_registers_in_Intel_x86_series
 *
 * @param value
 */
static inline __attribute__((always_inline)) void write_cr4(uintptr_t value)
{
    asm volatile ("mov %0, %%cr4" :: "r"(value));
}

// CR5 - 7 Reserved, same case as CR1.

