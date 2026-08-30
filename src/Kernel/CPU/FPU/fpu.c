#include "BalrogOS/CPU/FPU/fpu.h"
#include <stdint.h>

#include "BalrogOS/CPU/CR/control_register.h"

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
    size_t cr4 = read_cr4();
    /**
     * Enable : OSFXSR
     *
     * Operating system support for FXSAVE and FXRSTOR instructions
     *
     * If set, enables Streaming SIMD Extensions (SSE) instructions and fast FPU save & restore.
     *
     * See documentation : https://en.wikipedia.org/wiki/Control_register#CR3
     */
    cr4 |= 0x200;
    write_cr4(cr4);
    set_fpu_cw(0x37F);
}