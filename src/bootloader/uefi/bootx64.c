/*
The UEFI bootloader (required for modern hardware).

References :
UEFI       : https://wiki.osdev.org/UEFI
bare bones : https://wiki.osdev.org/UEFI_Bare_Bones
the spec   : https://uefi.org/specifications
*/

#include "balrog_os/boot/uefi_memory.h"
#include "balrog_os/boot/uefi_fs.h"
#include "balrog_os/boot/uefi_output.h"
#include "balrog_os/boot/uefi_boot_services.h"
#include "balrog_os/boot/uefi_system.h"
#include "balrog_os/boot/boot_framebuffer.h"
#include "uefi_layout.h"

static EFI_SYSTEM_TABLE* system_table = 0;
static EFI_BOOT_SERVICES* boot = 0;

static int uefi_firmware_is_gone = 0;

#define KERNEL_PHYS     0x8000
#define RAMFS_PHYS      0x10000000
#define SMAP_PHYS       0x7000
#define SMAP_COUNT_PHYS 0x6FFE
#define SMAP_MAX        64

#define FRAMEBUFFER_PHYS  0x7600
#define FRAMEBUFFER_MAGIC 0x42414C46

/*
Where we drop a copy of the rsdp.
*/
#define RSDP_PHYS     0x7C00
#define BDA_EBDA_PHYS 0x40E

/*
Upper_half adds KERNEL_OFFSET to rsp, so whatever we leave in it has to land
inside the 2MiB our tables map.
*/
#define BOOT_STACK_PHYS 0x6F00

#define PML4T_PHYS      0x1000
#define PAGE_PRESENT_RW 0x003

#define FB_PDPT_INDEX    511
#define FB_VIRTUAL_BASE  0xFFFFFFFFC0000000

/*
    OUTPUT
*/

static void __print(const char* str)
{
    CHAR16 wide[128];
    int i = 0;

    /*  the firmware speaks utf16 and we speak ascii, so we widen as we go.
        we do not handle anything above 127, we only print our own messages.  */
    while(str[i] && i < 126)
    {
        wide[i] = (CHAR16) str[i];
        i++;
    }

    wide[i] = 0;

    system_table->con_out->output_string(system_table->con_out, wide);
}

static void __print_hex(uint64_t value)
{
    const char* digits = "0123456789ABCDEF";
    char buffer[19];

    buffer[0] = '0';
    buffer[1] = 'x';

    for(int i = 0; i < 16; i++)
    {
        buffer[2 + i] = digits[(value >> ((15 - i) * 4)) & 0xF];
    }

    buffer[18] = 0;

    __print(buffer);
}

static void __fail(const char* message, EFI_STATUS status)
{
    if(!uefi_firmware_is_gone)
    {
        __print("balrog : ");
        __print(message);
        __print(" | status ");
        __print_hex(status);
        __print("\r\n");
    }

    while(1)
    {
        __asm__ volatile("hlt");
    }
}

/*
    FILES
*/

static EFI_FILE_PROTOCOL* __open_volume(EFI_HANDLE image)
{
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID file_system_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_LOADED_IMAGE_PROTOCOL* loaded = 0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* file_system = 0;
    EFI_FILE_PROTOCOL* root = 0;

    /* the loaded image tells us which device we were read from, and we want
       to read the kernel from there too. */
    EFI_STATUS status = boot->handle_protocol(image, &loaded_image_guid, (void**) &loaded);

    if(EFI_ERROR(status))
    {
        __fail("no loaded image protocol", status);
    }

    status = boot->handle_protocol(loaded->device_handle, &file_system_guid,
        (void**) &file_system);

    if(EFI_ERROR(status))
    {
        __fail("no file system on our own device", status);
    }

    status = file_system->open_volume(file_system, &root);

    if(EFI_ERROR(status))
    {
        __fail("cannot open the volume", status);
    }

    return root;
}

/*
Read a whole file into pages we allocate. We ask the file for its size first,
we cannot read into a buffer we did not size.
*/
static void* __read_file(EFI_FILE_PROTOCOL* root, CHAR16* name, uint64_t* out_size)
{
    EFI_GUID file_info_guid = EFI_FILE_INFO_GUID;
    EFI_FILE_PROTOCOL* file = 0;

    EFI_STATUS status = root->open(root, &file, name, EFI_FILE_MODE_READ, 0);

    if(EFI_ERROR(status))
    {
        __fail("cannot open a file", status);
    }

    uint8_t info_buffer[512];
    uint64_t info_size = sizeof(info_buffer);

    status = file->get_info(file, &file_info_guid, &info_size, info_buffer);

    if(EFI_ERROR(status))
    {
        __fail("cannot stat a file", status);
    }

    EFI_FILE_INFO* info = (EFI_FILE_INFO*) info_buffer;
    uint64_t size = info->file_size;
    uint64_t pages = (size + 4095) / 4096;

    EFI_PHYSICAL_ADDRESS buffer = 0;
    status = boot->allocate_pages(AllocateAnyPages, EfiLoaderData, pages, &buffer);

    if(EFI_ERROR(status))
    {
        __fail("cannot allocate for a file", status);
    }

    uint64_t read_size = size;
    status = file->read(file, &read_size, (void*) buffer);

    if(EFI_ERROR(status) || read_size != size)
    {
        __fail("short read", status);
    }

    file->close(file);

    *out_size = size;

    return (void*) buffer;
}

/*
    MEMORY MAP

The kernel reads e820 entries, so we translate.
source : https://wiki.osdev.org/Detecting_Memory_(x86)
*/
typedef struct
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) smap_entry_t;

static boot_framebuffer_t framebuffer = { 0, 0, 0, 0, 0, 0, 0 };

/*
source : https://wiki.osdev.org/GOP
*/
static void __find_framebuffer(void)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = 0;

    EFI_STATUS status = boot->locate_protocol(&gop_guid, 0, (void**) &gop);

    if(EFI_ERROR(status) || !gop || !gop->mode)
    {
        __print("no framebuffer, the kernel will use the text buffer\r\n");
        return;
    }

    /*
     * pixel_format 0 and 1 are the two 32bit layouts, rgb and bgr.
     * other format are not supported atm.
     */
    if(gop->mode->info->pixel_format > 1)
    {
        __print("framebuffer format we cannot draw on\r\n");
        return;
    }

    framebuffer.magic = FRAMEBUFFER_MAGIC;
    framebuffer.bpp = 32;
    framebuffer.address = gop->mode->frame_buffer_base;
    framebuffer.width = gop->mode->info->horizontal_resolution;
    framebuffer.height = gop->mode->info->vertical_resolution;
    framebuffer.pitch = gop->mode->info->pixels_per_scan_line;

    __print("framebuffer at ");
    __print_hex(framebuffer.address);
    __print("\r\n");
}

/*
    ACPI | finding the rsdp the firmware already knows about
*/

/*
Only the head of the rsdp, we do not read the tables here.
source : https://wiki.osdev.org/RSDP
*/
typedef struct
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
} __attribute__((packed)) rsdp_head_t;

static void* rsdp = 0;
static uint64_t rsdp_size = 0;

static int __guid_equal(EFI_GUID* a, EFI_GUID* b)
{
    if(a->data1 != b->data1 || a->data2 != b->data2 || a->data3 != b->data3)
    {
        return 0;
    }

    for(int i = 0; i < 8; i++)
    {
        if(a->data4[i] != b->data4[i])
        {
            return 0;
        }
    }

    return 1;
}

static void __find_rsdp(void)
{
    EFI_GUID acpi20 = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID acpi10 = EFI_ACPI_10_TABLE_GUID;
    void* fallback = 0;

    for(uint64_t i = 0; i < system_table->number_of_table_entries; i++)
    {
        EFI_CONFIGURATION_TABLE* entry = &system_table->configuration_table[i];

        if(__guid_equal(&entry->vendor_guid, &acpi20))
        {
            rsdp = entry->vendor_table;
            break;
        }

        if(__guid_equal(&entry->vendor_guid, &acpi10))
        {
            fallback = entry->vendor_table;
        }
    }

    if(!rsdp)
    {
        rsdp = fallback;
    }

    if(!rsdp)
    {
        __print("no acpi tables, the kernel will run without acpi\r\n");
        return;
    }

    rsdp_head_t* head = rsdp;

    rsdp_size = 20;

    if(head->revision >= 2 && head->length > 20 && head->length <= 64)
    {
        rsdp_size = head->length;
    }

    __print("rsdp at ");
    __print_hex((uint64_t) rsdp);
    __print("\r\n");
}

static uint32_t __smap_type(uint32_t efi_type)
{
    switch(efi_type)
    {
        case EfiConventionalMemory:
        case EfiLoaderCode:
        case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
            return 1;

        case EfiACPIReclaimMemory:
            return 3;

        case EfiACPIMemoryNVS:
            return 4;

        case EfiUnusableMemory:
            return 5;

        default:
            return 2;
    }
}

/*
    PAGING

The same four tables _PrepareKernel builds, at the same addresses, with the
same content. The kernel is linked for the higher half and Upper_half expects
to find them where they are.

    0x1000  PML4T   entry 0 and entry 511 both point to the PDPT
    0x2000  PDPT    entry 0 points to the PDT
    0x3000  PDT     entry 0 points to the PT
    0x4000  PT      512 entries, identity over the first 2MiB

Entry 511 of the PML4T is what makes the higher half | KERNEL_OFFSET is
0xFFFFFF8000000000 and its PML4 index is 511, so the kernel sees the first 2MiB
of physical memory at its own address.
*/
static void __build_page_tables(void)
{
    uint64_t* pml4t = (uint64_t*) PML4T_PHYS;
    uint64_t* pdpt = (uint64_t*) 0x2000;
    uint64_t* pdt = (uint64_t*) 0x3000;
    uint64_t* pt = (uint64_t*) 0x4000;

    for(int i = 0; i < 4096 / 8 * 4; i++)
    {
        ((uint64_t*) PML4T_PHYS)[i] = 0;
    }

    pml4t[0] = 0x2000 | PAGE_PRESENT_RW;
    pml4t[511] = 0x2000 | PAGE_PRESENT_RW;
    pdpt[0] = 0x3000 | PAGE_PRESENT_RW;
    pdt[0] = 0x4000 | PAGE_PRESENT_RW;

    for(uint64_t i = 0; i < 512; i++)
    {
        pt[i] = (i * 0x1000) | PAGE_PRESENT_RW;
    }

    if(!framebuffer.magic)
    {
        return;
    }

    uint64_t* fb_pdt = (uint64_t*) 0x5000;
    uint64_t fb_start = framebuffer.address & ~(uint64_t)0x1FFFFF;
    uint64_t fb_bytes = (uint64_t) framebuffer.pitch * framebuffer.height * 4;

    for(int i = 0; i < 512; i++)
    {
        fb_pdt[i] = 0;
    }

    pdpt[FB_PDPT_INDEX] = 0x5000 | PAGE_PRESENT_RW;

    for(uint64_t offset = 0; offset < fb_bytes + 0x200000; offset += 0x200000)
    {
        uint64_t index = offset >> 21;

        if(index >= 512)
        {
            break;
        }

        fb_pdt[index] = (fb_start + offset) | PAGE_PRESENT_RW | 0x80 | 0x10;
    }
}

static const uint8_t trampoline_code[] =
{
    0x48, 0xC7, 0xC4, 0x00, 0x6F, 0x00, 0x00,   /* mov rsp, BOOT_STACK    */
    0x48, 0xC7, 0xC0, 0x00, 0x10, 0x00, 0x00,   /* mov rax, 0x1000        */
    0x0F, 0x22, 0xD8,                           /* mov cr3, rax           */
    0x6A, 0x08,                                 /* push 0x08, GDT64.Code  */
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,   /* mov rax, LongMode      */
    0x50,                                       /* push rax               */
    0x48, 0xCB                                  /* lretq                  */
};

#define TRAMPOLINE_LONGMODE_OFFSET 22

static EFI_PHYSICAL_ADDRESS __prepare_trampoline(void)
{
    EFI_PHYSICAL_ADDRESS trampoline = 0x200000;
    EFI_STATUS status = boot->allocate_pages(AllocateMaxAddress, EfiLoaderCode,
        1, &trampoline);

    if(EFI_ERROR(status))
    {
        __fail("no page under 2MiB for the trampoline", status);
    }

    /*  __build_page_tables writes over 0x1000 to 0x5000, so the trampoline
        cannot live in there.  */
    if(trampoline >= 0x1000 && trampoline < 0x5000)
    {
        __fail("the firmware gave us a page inside the page tables", trampoline);
    }

    uint8_t* code = (uint8_t*) trampoline;

    for(uint64_t i = 0; i < sizeof(trampoline_code); i++)
    {
        code[i] = trampoline_code[i];
    }

    /*  the kernel is at KERNEL_PHYS and LongMode is at a fixed offset in it,
        which the makefile reads out of kernel.elf with nm.  */
    uint32_t long_mode = KERNEL_PHYS + KERNEL_LONGMODE_OFFSET;

    code[TRAMPOLINE_LONGMODE_OFFSET + 0] = long_mode & 0xFF;
    code[TRAMPOLINE_LONGMODE_OFFSET + 1] = (long_mode >> 8) & 0xFF;
    code[TRAMPOLINE_LONGMODE_OFFSET + 2] = (long_mode >> 16) & 0xFF;
    code[TRAMPOLINE_LONGMODE_OFFSET + 3] = (long_mode >> 24) & 0xFF;

    /*  the gdt lives inside kernel.bin and it is already at its address, so we
        load it here where the firmware still maps everything, and the
        trampoline only has cr3 and the jump left to do.  */
    struct __attribute__((packed))
    {
        uint16_t limit;
        uint64_t base;
    }* gdt_pointer = (void*) (uint64_t) (KERNEL_PHYS + KERNEL_GDT64_POINTER_OFFSET);

    (void) gdt_pointer;

    return trampoline;
}

static void __jump_to_kernel(EFI_PHYSICAL_ADDRESS trampoline)
{
    struct __attribute__((packed))
    {
        uint16_t limit;
        uint64_t base;
    }* gdt_pointer = (void*) (uint64_t) (KERNEL_PHYS + KERNEL_GDT64_POINTER_OFFSET);

    __asm__ volatile(
        "cli\n"
        "lgdt (%0)\n"
        "jmp *%1\n"
        :
        : "r"(gdt_pointer), "r"(trampoline)
        : "memory");
}

/*
    ENTRY
*/

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* table)
{
    system_table = table;
    boot = table->boot_services;

    __print("\r\nBalrogOS uefi loader\r\n");

    EFI_FILE_PROTOCOL* root = __open_volume(image);

    uint64_t kernel_size = 0;
    uint64_t ramfs_size = 0;

    CHAR16 kernel_name[] = { 'k','e','r','n','e','l','.','b','i','n', 0 };
    CHAR16 ramfs_name[] = { 'r','a','m','f','s','.','i','m','g', 0 };

    void* kernel = __read_file(root, kernel_name, &kernel_size);
    __print("kernel read\r\n");

    void* ramfs = __read_file(root, ramfs_name, &ramfs_size);
    __print("ramfs read\r\n");

    __find_framebuffer();

    /*  the configuration table belongs to the firmware, so we read it while
        the firmware is still there. the rsdp itself sits in acpi memory, which
        survives ExitBootServices, so the copy can wait until we own the low
        addresses.  */
    __find_rsdp();

    EFI_PHYSICAL_ADDRESS trampoline = __prepare_trampoline();

    /*  the map has to be asked for twice. the first call tells us how big it
        is, and allocating the buffer for it changes it, so we ask again.  */
    uint64_t map_size = 0;
    uint64_t map_key = 0;
    uint64_t descriptor_size = 0;
    uint32_t descriptor_version = 0;
    EFI_MEMORY_DESCRIPTOR* map = 0;

    boot->get_memory_map(&map_size, 0, &map_key, &descriptor_size, &descriptor_version);

    /*  room for the entries the allocation itself is going to add  */
    map_size += 4 * descriptor_size;

    EFI_STATUS status = boot->allocate_pool(EfiLoaderData, map_size, (void**) &map);

    if(EFI_ERROR(status))
    {
        __fail("cannot allocate the memory map", status);
    }

    /*  ExitBootServices only accepts the key of the map as it is right now, so
        we read it again and we leave immediately after, anything in between
        can move the map and make the key stale.
        source : https://wiki.osdev.org/UEFI  */
    for(int try = 0; try < 4; try++)
    {
        uint64_t size = map_size;

        status = boot->get_memory_map(&size, map, &map_key, &descriptor_size,
            &descriptor_version);

        if(EFI_ERROR(status))
        {
            __fail("cannot read the memory map", status);
        }

        map_size = size;
        status = boot->exit_boot_services(image, map_key);

        if(!EFI_ERROR(status))
        {
            break;
        }
    }

    if(EFI_ERROR(status))
    {
        __fail("cannot exit the boot services", status);
    }

    uefi_firmware_is_gone = 1;

    /*
        !!! FROM HERE THE FIRMWARE IS GONE !!!
    */

    smap_entry_t* smap = (smap_entry_t*) SMAP_PHYS;
    uint16_t* smap_count = (uint16_t*) SMAP_COUNT_PHYS;
    uint16_t count = 0;

    /*
    TODO : make the kernel understand EFI_MEMORY_DESCRIPTOR and not only smap.
    botch : atm I translate to smap
    */
    for(uint64_t offset = 0; offset < map_size && count < SMAP_MAX;
        offset += descriptor_size)
    {
        EFI_MEMORY_DESCRIPTOR* entry = (EFI_MEMORY_DESCRIPTOR*) ((uint8_t*) map + offset);

        if(entry->number_of_pages == 0)
        {
            continue;
        }

        uint64_t base = entry->physical_start;
        uint64_t length = entry->number_of_pages * 4096;
        uint32_t type = __smap_type(entry->type);

        if(count > 0
           && smap[count - 1].type == type
           && smap[count - 1].base + smap[count - 1].length == base)
        {
            smap[count - 1].length += length;
            continue;
        }

        smap[count].base = base;
        smap[count].length = length;
        smap[count].type = type;
        smap[count].acpi = 1;
        count++;
    }

    *smap_count = count;

    /* now that the memory is ours we can write it down where the kernel expect it. */
    *(boot_framebuffer_t*) FRAMEBUFFER_PHYS = framebuffer;

    /* same for rsdp */
    if(rsdp)
    {
        uint8_t* rsdp_destination = (uint8_t*) RSDP_PHYS;
        uint8_t* rsdp_source = rsdp;

        for(uint64_t i = 0; i < rsdp_size; i++)
        {
            rsdp_destination[i] = rsdp_source[i];
        }

        *(uint16_t*) (uint64_t) BDA_EBDA_PHYS = RSDP_PHYS / 16;
    }

    /* kernel address for the jump. */
    uint8_t* destination = (uint8_t*) KERNEL_PHYS;
    uint8_t* source = kernel;

    for(uint64_t i = 0; i < kernel_size; i++)
    {
        destination[i] = source[i];
    }

    destination = (uint8_t*) RAMFS_PHYS;
    source = ramfs;

    for(uint64_t i = 0; i < ramfs_size; i++)
    {
        destination[i] = source[i];
    }

    /*  _PrepareKernel writes these two out of ax and bx, we write them where
        it would have. the kernel reads them back in _KernelEntry and never
        knows which way it was started.  */
    *(uint16_t*) (uint64_t) (KERNEL_PHYS + KERNEL_MEMORY_INFO_OFFSET) = SMAP_PHYS;
    *(uint16_t*) (uint64_t) (KERNEL_PHYS + KERNEL_MEMORY_ENTRIES_OFFSET) = SMAP_COUNT_PHYS;

    __build_page_tables();
    __jump_to_kernel(trampoline);

    return EFI_SUCCESS;
}
