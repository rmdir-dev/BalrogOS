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
 * @brief 
 * 
 */
void init_vmm();

/**
 * @brief return the physical address of a given virtual address page
 * 
 * @param PT 
 * @param virt_addr 
 * @return uintptr_t 
 */
uintptr_t vmm_get_page(page_table* PML4T, uintptr_t virt_addr);

/**
 * @brief 
 * 
 * @param P4 
 * @param virt_addr 
 * @param phys_addr 
 * @param flags 
 */
void vmm_set_page(page_table* PML4T, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags);