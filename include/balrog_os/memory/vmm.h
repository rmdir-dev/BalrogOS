#pragma once
#include <stdint.h>

/*
Contain 4 arrays of 512 uint64_t ptr
PML4T = Page-map Level 4 Table
PDPT = Page directory Pointer Table
PDT = Page director Table
PT = Page table
*/
typedef uintptr_t page_table;

/**
 * @brief initialize virtual memory.
 * 
 */
int init_vmm();

/**
 * @brief get the physical address of a given virtual address
 * 
 * @param PML4T the PML4T currently in use, if none then the default (kernel) is use
 * @param virt_addr virtual address
 * @return void* the physical address of a given virtual address page
 */
void* vmm_get_page(page_table* PML4T, void* virt_addr);

/**
 * @brief Set a physical address into the page table at a given virtual address
 * 
 * @param PML4T the PML4T currently in use, if none then the default (kernel) is use
 * @param virt_addr the virtual address to set
 * @param phys_addr the physical address to place into the page table at virt_addr
 * @param flags Page flags to use.
 * @return the newly setted page
 */
void* vmm_set_page(page_table* PML4T, void* virt_addr, void* phys_addr, uint32_t flags);

/**
 * @brief 
 * 
 * @param PML4T 
 * @param virt_addr 
 */
void vmm_free_page(page_table* PML4T, void* virt_addr);

/**
 * @brief 
 * 
 * @param PML4T 
 * @return int 
 */
int vmm_clean_page_table(page_table* PML4T, uint8_t tables_only);

/**
 * @brief returns the kernel pml4t
 *
 * @return
 */
page_table* vmm_get_kernel_pml4t();

/**
 * @brief returns the kernel pdpt
 *
 * @return
 */
page_table* vmm_get_kernel_pdpt();

/**
 * @brief drop one page out of the tlb
 *
 * @param virt_addr the address whose translation must go
 */
static inline __attribute__((always_inline)) void vmm_invalidate(void* virt_addr)
{
    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}