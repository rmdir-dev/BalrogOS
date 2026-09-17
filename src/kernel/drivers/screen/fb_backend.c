#include "balrog_os/drivers/screen/fb_backend.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/drivers/screen/font_8x16.h"
#include "balrog_os/debug/debug_output.h"

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static uint32_t* fb_pixels = 0;
static uint32_t fb_pitch = 0;
static size_t fb_columns = 0;

/*  kept only so fb_log_info can say where the screen was  */
static uint64_t fb_phys = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;

/*  why fb_init gave up, held until there is somewhere to print it  */
static const char* fb_fail = 0;
static uint32_t fb_fail_value = 0;

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

int fb_init(size_t* width, size_t* height)
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

    *width = info->width / FONT_WIDTH;
    *height = info->height / FONT_HEIGHT;
    fb_columns = *width;

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
}

void fb_log_info(void)
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
}
