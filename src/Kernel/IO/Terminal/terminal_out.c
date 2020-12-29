#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "string.h"

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
 
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void clear_screen()
{
	terminal_color = vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
	terminal_buffer = (uintptr_t) 0xB8000 | 0xFFFFFF8000000000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	clear_screen();
}
 
void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void increase_terminal_row()
{
	if (++terminal_row == VGA_HEIGHT)
	{
			terminal_row = 0;
			clear_screen();
	}
}

void increase_terminal_column()
{
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		increase_terminal_row();
	}
}

void terminal_set_char_for_color(uint8_t hight_intensity, uint8_t color) 
{
	uint8_t base_color = 0;
	if(hight_intensity == 1)
	{
		base_color = 8;
	}
	terminal_setcolor(base_color + color);
}

void terminal_check_color(const char* data, size_t index)
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
			terminal_set_char_for_color(high_intensity, 0);
			break;
		case '1':
			terminal_set_char_for_color(high_intensity, 1);
			break;
		case '2':
			terminal_set_char_for_color(high_intensity, 2);
			break;
		case '3':
			terminal_set_char_for_color(high_intensity, 3);
			break;
		case '4':
			terminal_set_char_for_color(high_intensity, 4);
			break;
		case '5':
			terminal_set_char_for_color(high_intensity, 5);
			break;
		case '6':
			terminal_set_char_for_color(high_intensity, 6);
			break;
		case '7':
			terminal_set_char_for_color(high_intensity, 7);
			break;
		
		default:
			break;
		}
	}
}

size_t terminal_check_text(const char* data, size_t start_index)
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
				terminal_check_color(data, color_index);
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
		terminal_setcolor(VGA_COLOR_WHITE);
		start_index += 2;
	}

	return start_index;
}
 
void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
	{
		switch (data[i])
		{
		case '\n':
			increase_terminal_row();
			terminal_column = 0;
			break;

		case '\r':
			//TODO
			break;

		case '\t':
			//TODO
			break;

			// COLOR
		case '\e':
			i = terminal_check_text(data, i);
			break;
		
		default:
		{
			const size_t index = terminal_row * VGA_WIDTH + terminal_column;
			terminal_buffer[index] = vga_entry(data[i], terminal_color);

			increase_terminal_column();
		}
			break;
		}
	}
}
 
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}