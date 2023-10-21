#include "BalrogOS/CPU/FPU/fpu.h"
#include <stdint.h>

/**
 * @brief Set the FPU control word
 *
 * @param cw new control word value.
 */
void set_fpu_cw(const uint16_t cw) {
    asm volatile("fldcw %0" :: "m"(cw));
}

/**
 * @brief Initialize the FPU (Floating Point Unit)
 */
void init_fpu() {
    size_t cr4;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x200;
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));
    set_fpu_cw(0x37F);
}