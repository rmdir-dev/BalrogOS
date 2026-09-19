#pragma once
#include <stdint.h>
#include <stddef.h>
#include "balrog/debug/debug.h"
#include "balrog_os/file_system/fs_devices.h"

enum klog_logging_method
{
    KLOG_SERIAL_LOG = 0x00,
    KLOG_RAM_LOG    = 0x01,
    KLOG_DISK_LOG   = 0x02,
    KLOG_USB_LOG    = 0x03,
};

/**
 * @param str the data to print
 * @param size the size of the data
 */
typedef void (*klog_write_callback) (const char *str, size_t size);

void klog_write(enum klog_logging_level log_level, const char *str, size_t size);

int klog_set_default_handler(klog_write_callback write_callback, enum klog_logging_level log_level, enum klog_logging_method method);

int klog_register_handler(klog_write_callback write_callback, enum klog_logging_level log_level, enum klog_logging_method method);

int klog_register_fs_device(fs_device_t* device, enum klog_logging_level log_level, enum klog_logging_method method);

void klog_claim_buffer();

void klog_force_flush_buffers();

int init_klog();

void wormtongue();