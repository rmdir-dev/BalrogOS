#pragma once
#include <stddef.h>
#include <stdint.h>
#include "balrog_os/boot/boot_framebuffer.h"

/*
References :
linear framebuffer : https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer
GOP                : https://wiki.osdev.org/GOP
*/

#define BOOT_FRAMEBUFFER_PHYS   0x7600
#define BOOT_FRAMEBUFFER_MAGIC  0x42414C46      /* "FLAB" read as bytes */

#define FB_VIRTUAL_BASE         0xFFFFFFFFC0000000

/**
 * @brief
 *
 * @param width
 * @param height
 * @return
 */
int fb_init(size_t* width, size_t* height);

/**
 * @brief
 *
 * @param
 * @param
 * @param
 */
void fb_put(size_t index, unsigned char uc, uint8_t color);

/**
 * @brief
 */
void fb_log_info(void);
