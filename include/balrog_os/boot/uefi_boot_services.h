#pragma once
#include "balrog_os/boot/uefi.h"
#include "balrog_os/boot/uefi_memory.h"

/*
    BOOT SERVICES

OSDev, UEFI § Convenience functions : https://wiki.osdev.org/UEFI#Convenience_functions

UEFI firmware establishes many callable functions in memory, which are grouped
into sets called "protocols" and are discoverable through the System Table.
The behavior of each function in each protocol is defined by specification.

The order are important as moving one would move the address of the function
pointers.
*/

typedef struct
{
    EFI_TABLE_HEADER hdr;

    void* raise_tpl;
    void* restore_tpl;

    EFI_STATUS (EFIAPI *allocate_pages)(EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE memory_type, uint64_t pages,
        EFI_PHYSICAL_ADDRESS* memory);
    EFI_STATUS (EFIAPI *free_pages)(EFI_PHYSICAL_ADDRESS memory, uint64_t pages);
    EFI_STATUS (EFIAPI *get_memory_map)(uint64_t* memory_map_size, EFI_MEMORY_DESCRIPTOR* memory_map, uint64_t* map_key,
        uint64_t* descriptor_size, uint32_t* descriptor_version);
    EFI_STATUS (EFIAPI *allocate_pool)(EFI_MEMORY_TYPE pool_type, uint64_t size, void** buffer);
    EFI_STATUS (EFIAPI *free_pool)(void* buffer);

    void* create_event;
    void* set_timer;
    void* wait_for_event;
    void* signal_event;
    void* close_event;
    void* check_event;

    void* install_protocol_interface;
    void* reinstall_protocol_interface;
    void* uninstall_protocol_interface;
    EFI_STATUS (EFIAPI *handle_protocol)(EFI_HANDLE handle, EFI_GUID* protocol, void** interface);
    void* reserved;
    void* register_protocol_notify;
    void* locate_handle;
    void* locate_device_path;
    void* install_configuration_table;

    void* load_image;
    void* start_image;
    void* exit;
    void* unload_image;
    EFI_STATUS (EFIAPI *exit_boot_services)(EFI_HANDLE image_handle, uint64_t map_key);

    void* get_next_monotonic_count;
    EFI_STATUS (EFIAPI *stall)(uint64_t microseconds);
    void* set_watchdog_timer;

    void* connect_controller;
    void* disconnect_controller;

    void* open_protocol;
    void* close_protocol;
    void* open_protocol_information;

    void* protocols_per_handle;
    void* locate_handle_buffer;
    EFI_STATUS (EFIAPI *locate_protocol)(EFI_GUID* protocol, void* registration, void** interface);
    void* install_multiple_protocol_interfaces;
    void* uninstall_multiple_protocol_interfaces;

    void* calculate_crc32;

    void* copy_mem;
    void* set_mem;
    void* create_event_ex;
} EFI_BOOT_SERVICES;
