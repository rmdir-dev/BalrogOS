#pragma once
#include "balrog_os/boot/uefi.h"

/*
    FILE PROTOCOL

OSDev, Loading files under UEFI : https://wiki.osdev.org/Loading_files_under_UEFI

UEFI is supposed to provide an easy way of loading files from partitions.
Sadly it is not so easy, considerably more complicated than reading sectors,
but at least it parses the file system for you.

OSDev, Loading files under UEFI § Read Data from File :
https://wiki.osdev.org/Loading_files_under_UEFI#Read_Data_from_File

Now that we have a Volume instance, it's rather easy to use it. It has the
classic Open / Read / Close abstraction.

OSDev, Loading files under UEFI § Open :
https://wiki.osdev.org/Loading_files_under_UEFI#Open

Note that the file name passed to Volume->Open is a UNICODE16 string, meaning
all characters are 2 bytes long. To get the "L" string literal correctly,
you'll need to pass the "-fshort-wchar" command line option to gcc.
*/

#define EFI_FILE_MODE_READ  0x0000000000000001
#define EFI_FILE_MODE_WRITE 0x0000000000000002

struct _EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
    struct _EFI_FILE_PROTOCOL* self, struct _EFI_FILE_PROTOCOL** new_handle,
    CHAR16* file_name, uint64_t open_mode, uint64_t attributes);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(struct _EFI_FILE_PROTOCOL* self);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(
    struct _EFI_FILE_PROTOCOL* self, uint64_t* buffer_size, void* buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(
    struct _EFI_FILE_PROTOCOL* self, uint64_t position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(
    struct _EFI_FILE_PROTOCOL* self, EFI_GUID* information_type,
    uint64_t* buffer_size, void* buffer);

typedef struct _EFI_FILE_PROTOCOL
{
    uint64_t revision;
    EFI_FILE_OPEN open;
    EFI_FILE_CLOSE close;
    void* delete_file;
    EFI_FILE_READ read;
    void* write;
    void* get_position;
    EFI_FILE_SET_POSITION set_position;
    EFI_FILE_GET_INFO get_info;
    void* set_info;
    void* flush;
} EFI_FILE_PROTOCOL;

/*
    FILE INFO

OSDev, Loading files under UEFI § Get File Size :
https://wiki.osdev.org/Loading_files_under_UEFI#Get_File_Size

In order to know how much to read (ReadSize variable above), you'll need to
know the file's size. For that, you should use FileHandle->GetInfo. The problem
is, there's no way of knowing how big buffer the information structure
requires, so you'll have to grow the buffer dynamically if GetInfo fails.
*/
#define EFI_FILE_INFO_GUID \
    { 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

typedef struct
{
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    uint8_t create_time[16];
    uint8_t last_access_time[16];
    uint8_t modification_time[16];
    uint64_t attribute;
    CHAR16 file_name[1];
} EFI_FILE_INFO;

/*
    SIMPLE FILE SYSTEM

OSDev, Loading files under UEFI § Volume Handle :
https://wiki.osdev.org/Loading_files_under_UEFI#Volume_Handle

There's no common "Open File" function in UEFI. Instead each volume has its
own. So the first thing you need to do is locate a volume handle.
*/

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* self, EFI_FILE_PROTOCOL** root);

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
{
    uint64_t revision;
    EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME open_volume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
