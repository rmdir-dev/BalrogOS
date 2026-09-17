#pragma once
#include "balrog_os/boot/uefi.h"

/*
SIMPLE TEXT OUTPUT

OSDev, UEFI Bare Bones § hello.c : https://wiki.osdev.org/UEFI_Bare_Bones#hello.c

"// EFI Applications use Unicode and CRLF, a la Windows"
*/

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* self, BOOLEAN extended_verification);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* self, CHAR16* string);

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
    EFI_TEXT_RESET reset;
    EFI_TEXT_STRING output_string;
    void* test_string;
    void* query_mode;
    void* set_mode;
    void* set_attribute;
    void* clear_screen;
    void* set_cursor_position;
    void* enable_cursor;
    void* mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/*
GRAPHICS OUTPUT

OSDev, GOP § Graphics Output Protocol : https://wiki.osdev.org/GOP#Graphics_Output_Protocol

"It has basically the same functions as VESA, you can query the modes, set the
modes.  It also provides an efficient BitBlitter function, which you can't use
from your OS unfortunately. GOP is an EFI Boot Time Service, meaning you can't
access it after you call ExitBootServices().  However, the framebuffer provided
by GOP persists, so you can continue to use it for graphics output in your OS."

OSDev, GOP § Detecting GOP : https://wiki.osdev.org/GOP#Detecting_GOP

"As with other UEFI protocols, you have to locate a structure with the function
pointers first using the protocol's GUID."
*/

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }

/*
OSDev, GOP § Set Video Mode and Get the Framebuffer :
https://wiki.osdev.org/GOP#Set_Video_Mode_and_Get_the_Framebuffer

"To get the same value as scanline in VESA (also commonly called pitch in many
graphics libraries), you have to multiply PixelsPerScanLine by the number of
bytes per pixel. That can be detected by examining the
gop->Mode->Info->PixelFormat field."
*/
typedef struct
{
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
    uint32_t pixel_information[4];
    uint32_t pixels_per_scan_line;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct
{
    uint32_t max_mode;
    uint32_t mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    uint64_t size_of_info;
    EFI_PHYSICAL_ADDRESS frame_buffer_base;
    uint64_t frame_buffer_size;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct
{
    void* query_mode;
    void* set_mode;
    void* blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;
