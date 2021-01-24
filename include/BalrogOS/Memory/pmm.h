#pragma once
#include "memory.h"

/**
 * @brief Free a page of physical memory
 * 
 * @param addr the memory address.
 */
void pmm_free(uintptr_t* addr);

/**
 * @brief allocate a new page of physical memory
 * 
 * @return uintptr_t the physical address of the new allocated page
 */
uintptr_t* pmm_alloc();

/**
 * @brief allocate a new page of physical memory and set it to 0
 * 
 * @return uintptr_t the physical address of the new allocated page
 */
uintptr_t* pmm_calloc();

/**
 * @brief initialize the physical memory manager
 * 
 * @param SMAPinfo 
 * @param SMAPsize 
 */
void pmm_init(SMAP_entry* SMAPinfo, uint16_t* SMAPsize);