#pragma once
#include <stddef.h>
#include <stdint.h>
#include "balrog_os/boot/boot_framebuffer.h"
#include "balrog_os/drivers/screen/vga_driver.h"

/*
References :
linear framebuffer : https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer
GOP                : https://wiki.osdev.org/GOP
*/

#define BOOT_FRAMEBUFFER_PHYS   0x7600
#define BOOT_FRAMEBUFFER_MAGIC  0x42414C46      /* "FLAB" read as bytes */

#define FB_VIRTUAL_BASE         0xFFFFFFFFC0000000

/*
How many cells the two buffers can hold, which caps the resolution we buffer.

Resolution of 3840 x 2160
(3840 / 8) x (2160 / 16) => 480 * 135 = 64 800 cells
Then the next ^2 above it -> 65536
Number of buffers * MaxScreenCells * 2 bytes (16bit)
2 * 65536 * 2 = 256 KiB
*/
#define SCREEN_MAX_CELLS 65536

#define FB_BUFFERS_SIZE (SCREEN_MAX_CELLS * 2 * sizeof(uint16_t))

/**
 * @brief
 *
 * @param width
 * @param height
 * @return
 */
int fb_init(size_t* width, size_t* height, screen_actions_t* screen_actions);

/**
 * @brief
 *
 * @param
 * @param
 * @param
 */
void fb_put(size_t index, unsigned char uc, uint8_t color);

/**
 *
 * @param index
 * @param uc
 */
void fb_put_buffered(size_t index, uint16_t uc);

/**
 *
 * @param index
 * @param uc
 * @param color
 */
void fb_screen_set(size_t index, unsigned char uc, uint8_t color);

/**
 *
 * @param color
 */
void fb_clear_back(uint8_t color);

/**
 * @brief
 *
 * @param color
 */
void fb_screen_clear(uint8_t color);

/**
 *
 */
void fb_flush_buffered();

/**
 * @brief keep the physical memory allocator off the two screen buffers
 * 
 */
void fb_claim_memory();

/**
 * @brief
 *
 */
void fb_log_info();
