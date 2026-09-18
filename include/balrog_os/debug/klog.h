#pragma once
#include <stdint.h>
#include <stddef.h>

enum klog_logging_method
{
    KLOG_SERIAL_LOG = 0x00,
    KLOG_RAM_LOG    = 0x01,
    KLOG_DISK_LOG   = 0x02,
    KLOG_USB_LOG    = 0x03,
};

enum klog_logging_level
{
    KLOG_VERBOSE    = 0x00,
    KLOG_INFO       = 0x01,
    KLOG_WARNING    = 0x02,
    KLOG_ERROR      = 0x03,
    KLOG_CRITICAL   = 0x04,
    KLOG_FATAL      = 0x05,
};

typedef void (*klog_write_callback) (const char *str, size_t size);

void klog_write(const char *str, size_t size);

int klog_set_default_handler(klog_write_callback write_callback, enum klog_logging_method method);

int klog_register_handler(klog_write_callback write_callback, enum klog_logging_method method);

void klog_claim_buffer();

int init_klog();