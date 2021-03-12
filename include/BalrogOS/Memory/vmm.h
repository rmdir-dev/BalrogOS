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
void init_vmm();

/**
 * @brief get the physical address of a given virtual address
 * 
 * @param PML4T the PML4T currently in use, if none then the default (kernel) is use
 * @param virt_addr virtual address
 * @return uintptr_t the physical address of a given virtual address page
 */
uintptr_t vmm_get_page(page_table* PML4T, uintptr_t virt_addr);

/**
 * @brief Set a physical address into the page table at a given virtual address
 * 
 * @param PML4T the PML4T currently in use, if none then the default (kernel) is use
 * @param virt_addr the virtual address to set
 * @param phys_addr the physical address to place into the page table at virt_addr
 * @param flags Page flags to use.
 * @return the newly setted page
 */
void* vmm_set_page(page_table* PML4T, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags);

/**
 * @brief 
 * 
 * @param PML4T 
 * @param virt_addr 
 */
void vmm_free_page(page_table* PML4T, uintptr_t virt_addr);

/**
 * @brief 
 * 
 * @param PML4T 
 * @return int 
 */
int vmm_clean_page_table(page_table* PML4T);