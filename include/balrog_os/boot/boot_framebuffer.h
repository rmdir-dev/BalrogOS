#pragma once

#include <stdint.h>

typedef struct __boot_framebuffer_t
{
    /*
    Must be set to BOOT_FRAMEBUFFER_MAGIC by the loader.
    Anything else means there is no framebuffer and we are on the bios side.
    */
    uint32_t magic;
    /*
    How many bytes one pixel takes. We only handle 32.
    */
    uint32_t bpp;
    /*
    Physical address of the first pixel.
    */
    uint64_t address;
    uint32_t width;
    uint32_t height;
    /*
    How many pixels there are from the start of a line to the start of the
    next one. It is not always the width as the firmware pads the lines.
    */
    uint32_t pitch;
    uint32_t reserved;
} __attribute__((packed)) boot_framebuffer_t;