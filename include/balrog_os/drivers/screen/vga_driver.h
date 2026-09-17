#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct __screen_actions_t
{
    void (*write)(size_t index, unsigned char uc, uint8_t color);
    void (*clear)(uint8_t color);
    void (*clear_back)(uint8_t color);
    void (*flush)();
} screen_actions_t;

/**
 * @brief Initialize the VGA driver.
 * 
 */
int vga_init();

/**
 * @brief Write a string to the screen
 * 
 * @param data the string/data to print
 * @param size the size of the string
 */
void vga_write(const char* data, size_t size);

/**
 * @brief Clear the screen.
 * 
 */
void vga_clear();