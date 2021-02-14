#pragma once

#include <stddef.h>

/**
 * @brief Initialize the VGA driver.
 * 
 */
void vga_init();

/**
 * @brief Write a string to the screen
 * 
 * @param data the string/data to print
 * @param size the size of the string
 */
void vga_write(const char* data, size_t size);