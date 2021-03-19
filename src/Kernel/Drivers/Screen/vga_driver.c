#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include <stddef.h>
#include <stdint.h>
#include "BalrogOS/Memory/memory.h"
#include <string.h>

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

/**
 * @brief Change the background and foreground color
 * 		  Both color MUST be selected using the enum vga_color
 * 
 * @param fg fore ground color
 * @param bg background color
 * @return uint8_t the foregroud and background color combined
 */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

/**
 * @brief print a new character to the screen
 * 
 * @param uc the character to print
 * @param color the color it said character
 * @return uint16_t return the character and it's color to put into the screen buffer
 */
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t vga_row;
size_t vga_column;
uint8_t vga_color;
uint16_t* vga_buffer;

/**
 * @brief Clear the screen.
 * 
 */
static void vga_clear()
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			vga_buffer[index] = vga_entry(' ', vga_color);
		}
	}
}

/**
 * @brief Set the new VGA char color
 * 
 * @param color new color
 */
static inline void vga_setcolor(uint8_t color) 
{
	vga_color = color;
}

/**
 * @brief increase the row on the screen
 * 
 */
static void increase_vga_row()
{
	if (++vga_row == VGA_HEIGHT)
	{
		vga_row = 0;
	}
}

/**
 * @brief increase the screen column
 * 
 */
static void increase_vga_column()
{
	if (++vga_column == VGA_WIDTH) {
		vga_column = 0;
		increase_vga_row();
	}
}

/**
 * @brief Set a new character color
 * 
 * @param high_intensity 1 if high intensity color
 * @param color color
 */
static inline void vga_set_char_for_color(uint8_t high_intensity, uint8_t color) 
{
	uint8_t base_color = 0;
	if(high_intensity == 1)
	{
		base_color = 8;
	}
	vga_setcolor(base_color + color);
}

static void vga_check_color(const char* data, size_t index)
{
	uint8_t high_intensity = -1;
	switch (data[index])
	{
	case '3':
		high_intensity = 0;
		break;

	case '9':
		high_intensity = 1;
		break;
	
	default:
		break;
	}

	if(high_intensity != -1)
	{
		switch (data[index + 1])
		{
		case '0':
			vga_set_char_for_color(high_intensity, 0);
			break;
		case '1':
			vga_set_char_for_color(high_intensity, 1);
			break;
		case '2':
			vga_set_char_for_color(high_intensity, 2);
			break;
		case '3':
			vga_set_char_for_color(high_intensity, 3);
			break;
		case '4':
			vga_set_char_for_color(high_intensity, 4);
			break;
		case '5':
			vga_set_char_for_color(high_intensity, 5);
			break;
		case '6':
			vga_set_char_for_color(high_intensity, 6);
			break;
		case '7':
			vga_set_char_for_color(high_intensity, 7);
			break;
		
		default:
			break;
		}
	}
}

static size_t vga_check_text(const char* data, size_t start_index)
{
	start_index++;
	size_t color_index = 0;

	if(data[start_index] == '[' && data[start_index + 2] == ';')
	{
		switch (data[start_index + 1])
		{
		case '0':
			color_index = start_index + 3;
			start_index += 4;
			if(data[color_index + 2] == 'm')
			{
				start_index++;
				vga_check_color(data, color_index);
			}
			break;
		
		case '1':
			/* code */
			break;

		case '4':
			/* code */
			break;
		
		default:
			break;
		}
	} else if(data[start_index] == '[' && data[start_index + 1] == '0' && data[start_index + 2] == 'm')
	{
		vga_setcolor(VGA_COLOR_WHITE);
		start_index += 2;
	}

	return start_index;
}

void vga_init()
{
    vga_row = 0;
	vga_column = 0;
	vga_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
	vga_buffer = (uint16_t*) P2V(0xb8000);
	vga_clear();
}

void vga_write(const char* data, size_t size)
{
	if(vga_row == 0 && vga_column ==0)
	{
		vga_clear();
	}
	
    for (size_t i = 0; i < size; i++)
	{
		switch (data[i])
		{
		case '\n':
			increase_vga_row();
			vga_column = 0;
			break;

		case '\r':
			//TODO
			break;

		case '\t':
			//TODO
			break;

			// COLOR
		case '\e':
			i = vga_check_text(data, i);
			break;
		
		default:
		{
			const size_t index = vga_row * VGA_WIDTH + vga_column;
			vga_buffer[index] = vga_entry(data[i], vga_color);

			increase_vga_column();
			break;
		}
		}
	}
}