#include "balrog_os/drivers/screen/fb_backend.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/drivers/screen/font_8x16.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/pmm.h"
#include <string.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static uint16_t* fb_screen_back_buffer = 0;
static uint16_t* fb_screen_front_buffer = 0;
// enable screen only when the memory is properly reserved.
static uint8_t fb_screen_buffered = 0;

static uint32_t* fb_pixels = 0;
static uint32_t fb_pitch = 0;
static size_t fb_columns = 0;

/*  kept only so fb_log_info can say where the screen was  */
static uint64_t fb_phys = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_cell_width = 0;
static uint32_t fb_cell_height = 0;

/*  why fb_init gave up, held until there is somewhere to print it  */
static const char* fb_fail = 0;
static uint32_t fb_fail_value = 0;

/* logging */
static uint64_t fb_writes = 0;
static uint64_t fb_flushes = 0;
static uint8_t fb_logging = 0;

/*
source : https://en.wikipedia.org/wiki/VGA_text_mode
*/
static const uint32_t fb_palette[16] =
{
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static void __fb_wipe()
{
    for(uint32_t y = 0; y < fb_height; y++)
    {
        uint32_t* line = fb_pixels + (size_t) y * fb_pitch;

        for(uint32_t x = 0; x < fb_pitch; x++)
        {
            line[x] = 0;
        }
    }
}

int fb_init(size_t* width, size_t* height, screen_actions_t* screen_actions)
{
    boot_framebuffer_t* info = (boot_framebuffer_t*) P2V(BOOT_FRAMEBUFFER_PHYS);

    if(info->magic != BOOT_FRAMEBUFFER_MAGIC)
    {
        fb_fail = "no framebuffer handed over by the loader, magic";
        fb_fail_value = info->magic;
        return -1;
    }

    /*  we only know how to write a pixel as one 32 bit word. */
    if(info->bpp != 32)
    {
        fb_fail = "we only draw 32 bit pixels, the firmware gave bpp";
        fb_fail_value = info->bpp;
        return -1;
    }

    // vga_init is done before init_pmm so there is no allocator yet.
    fb_pixels = (uint32_t*) (FB_VIRTUAL_BASE + (info->address & 0x1FFFFF));
    fb_pitch = info->pitch;
    fb_phys = info->address;
    fb_width = info->width;
    fb_height = info->height;

    fb_cell_width = info->width / FONT_WIDTH;
    fb_cell_height = info->height / FONT_HEIGHT;
    *width = fb_cell_width;
    *height = fb_cell_height;
    fb_columns = *width;

    screen_actions->write = fb_screen_set;
    screen_actions->clear = fb_screen_clear;
    screen_actions->flush = fb_flush_buffered;
    screen_actions->clear_back = fb_clear_back;

    /*  before anything else draws, what is in there is not ours  */
    __fb_wipe();

    // Buffer the screen if we can accept it (for now it accept up to 4K screens).
    if ((fb_cell_width * fb_cell_height) <= SCREEN_MAX_CELLS)
    {
        /*  from the linker script, so everything that lives above the image
            is laid out in one file  */
        extern uintptr_t screen_buffers;
        extern uintptr_t screen_buffers_end;

        if((uintptr_t)&screen_buffers_end - (uintptr_t)&screen_buffers < FB_BUFFERS_SIZE)
        {
            fb_fail = "the .screen section is too small for SCREEN_MAX_CELLS, needs";
            fb_fail_value = FB_BUFFERS_SIZE;
            return 0;
        }

        fb_screen_back_buffer = (uint16_t*) &screen_buffers;
        fb_screen_front_buffer = fb_screen_back_buffer + SCREEN_MAX_CELLS;

        /* the bss is zeroed for us, .screen is NOLOAD so it's not. */
        memset(fb_screen_back_buffer, 0, FB_BUFFERS_SIZE);

        fb_screen_buffered = 1;
    }

    return 0;
}

void fb_put(size_t index, unsigned char uc, uint8_t color)
{
    if(!fb_pixels)
    {
        return;
    }

    /*  the terminal counts in cells, we count in pixels  */
    size_t column = index % fb_columns;
    size_t row = index / fb_columns;

    uint32_t foreground = fb_palette[color & 0x0F];
    uint32_t background = fb_palette[(color >> 4) & 0x0F];

    /*  the vga hardware draws the character 0 as a blank, but the font 0 is copyright.  */
    const uint8_t* glyph = font_8x16[uc ? uc : ' '];

    for(size_t y = 0; y < FONT_HEIGHT; y++)
    {
        uint32_t* line = fb_pixels + (row * FONT_HEIGHT + y) * fb_pitch
                + column * FONT_WIDTH;

        /*  a glyph line is one byte, the leftmost pixel is the high bit  */
        for(size_t x = 0; x < FONT_WIDTH; x++)
        {
            line[x] = (glyph[y] & (0x80 >> x)) ? foreground : background;
        }
    }

    fb_writes += FONT_WIDTH * FONT_HEIGHT;
}

void fb_put_buffered(size_t index, uint16_t uc)
{
    fb_screen_back_buffer[index] = uc;
}

static void __fb_flush_log()
{
    if (fb_logging || ++fb_flushes < 60)
    {
        return;
    }

    fb_logging = 1;

    kernel_debug_output(KDB_LVL_VERBOSE, "screen : %d write over %d flushes, %d each\n",
            fb_writes, fb_flushes, fb_writes / fb_flushes);
    fb_logging = 0;

    fb_writes = 0;
    fb_flushes = 0;
}

void fb_flush_buffered()
{
    if (!fb_screen_buffered)
    {
        return;
    }

    __fb_flush_log();

    size_t total_cells = fb_cell_width * fb_cell_height;

    for(size_t i = 0; i < total_cells; i++)
    {
        // if the character didn't change -> don't update.
        if (fb_screen_back_buffer[i] == fb_screen_front_buffer[i])
        {
            continue;
        }
        // update the character
        fb_put(i, fb_screen_back_buffer[i] & 0xFF, fb_screen_back_buffer[i] >> 8);
        // set the value of the back buffer into the front.
        fb_screen_front_buffer[i] = fb_screen_back_buffer[i];
    }
}

void fb_screen_set(size_t index, unsigned char uc, uint8_t color)
{
    if (index >= fb_cell_width * fb_cell_height)
    {
        return;
    }

    // if the screen is not buffered, simply send the values to screen_put
    if (!fb_screen_buffered)
    {
        fb_put(index, uc, color);
        return;
    }

    // else write it into the buffer
    // uc 0 = copyright symbol, so set it back to a space.
    fb_put_buffered(index, (uc ? uc : ' ') | ((uint16_t) color << 8));
}

void fb_clear_back(uint8_t color)
{
    size_t total_cells = fb_cell_width * fb_cell_height;

    for(size_t i = 0; i < total_cells; i++)
    {
        fb_screen_set(i, ' ', color);
    }
}

void fb_screen_clear(uint8_t color)
{
    fb_clear_back(color);
    fb_flush_buffered();
}

void fb_claim_memory()
{
    if(!fb_screen_buffered)
    {
        return;
    }

    // from linkerScript
    extern uintptr_t screen_buffers;

    pmm_reserve((void*)V2P(&screen_buffers), FB_BUFFERS_SIZE);
}

void fb_log_info()
{
    if(!fb_pixels)
    {
        if(fb_fail)
        {
            kernel_debug_output(KDB_LVL_INFO, "framebuffer : %s 0%x, staying on the text buffer",
                    fb_fail, fb_fail_value);
        }
        return;
    }

    KERNEL_LOG_INFO("framebuffer at 0%p, %dx%d, pitch %d",
            fb_phys, fb_width, fb_height, fb_pitch);
    kernel_debug_output(KDB_LVL_INFO, "framebuffer : %d columns, %d rows of %dx%d glyphs",
            fb_columns, fb_height / FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT);
    kernel_debug_output(KDB_LVL_INFO, "framebuffer : %d cells of %d, double buffer %s",
            fb_cell_width * fb_cell_height, SCREEN_MAX_CELLS,
            fb_screen_buffered ? "on" : "off, the mode is wider than the arrays");
}
