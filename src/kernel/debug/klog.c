#include "balrog_os/debug/klog.h"

#include "string.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/pmm.h"
#include "klib/data_structure/list.h"
#include "klib/data_structure/vector.h"


/*
TODO :
- wormtongue :
    - clear the disabled handlers
    - flush logs overtime on its own thread and thus avoiding auto flushing on running threads
    - compress the logs
 */

// #define MAX_BUFFER_SIZE (PAGE_SIZE * 16)    // 64KiB
#define MAX_BUFFER_SIZE (PAGE_SIZE * 16 * 8)    // 512KiB

typedef struct __klog_device_buffer_t
{
    uint8_t require_flush;
    uint8_t flushing;
    size_t current_pos;
    uint8_t* buffer;
} klog_device_buffer_t;

typedef struct __klog_debug_device_t
{
    fs_device_t* device;
    size_t start_lba;
    size_t end_lba;
    size_t current_lba;
    kmutex_t write_lock;
    int write_in_flight;
    size_t selected_buffer;
    klog_device_buffer_t buffers[2];
} klog_debug_device_t;

#define get_selected_buffer(kdb_device)     ((kdb_device)->buffers[(kdb_device)->selected_buffer])

typedef struct __klog_handler_t
{
    klog_write_callback write;
    klog_debug_device_t* device;
    enum klog_logging_method log_method;
    enum klog_logging_level log_level;
    size_t index;
    // TODO : wormtongue need to clear the disabled handlers from time to time.
    int disabled;
} klog_handler_t;

static uint8_t* klog_vstart = 0;
static uint8_t* klog_vend = 0;
static klog_handler_t klog_default_handler = {};
static size_t klog_area_index = 0;
static kmutex_t klog_flush_lock;

static vector_t klog_callbacks_vector = {
    .current_size = 0,
    .data = 0,
    .__data_size = 0,
    .__max_size = 0,
    .__mem_size = 0,
    .__growth = 0
};

static int __klog_safe_device_write(klog_debug_device_t* dbg_dev, uint8_t* buffer, size_t size)
{
    size_t lba_len = (size / SECTOR_SIZE);
    uint32_t last_written_lba = dbg_dev->current_lba + lba_len;
    if (last_written_lba >= dbg_dev->end_lba)
    {
        return -1;
    }

    int write_status = dbg_dev->device->write(dbg_dev->device, (uint8_t*) buffer, dbg_dev->current_lba, lba_len);

    if (write_status != 0)
    {
        return -1;
    }

    dbg_dev->current_lba = last_written_lba;

    return 0;
}

static int __klog_debug_device_write(klog_handler_t* handler, const char *str, size_t size)
{
    klog_debug_device_t* dbg_dev = handler->device;
    if (!dbg_dev)
    {
        handler->disabled = 1;
        kernel_debug_output(KDB_LVL_ERROR, "klog: no device found !");
        return -1;
    }

    if((get_selected_buffer(dbg_dev).current_pos + size) < MAX_BUFFER_SIZE)
    {
        memcpy(get_selected_buffer(dbg_dev).buffer + get_selected_buffer(dbg_dev).current_pos, str, size);
        get_selected_buffer(dbg_dev).current_pos += size;
    } else
    {
        size_t i = 0;
        klog_device_buffer_t* selected_buffer = &get_selected_buffer(dbg_dev);

        if (selected_buffer->require_flush || selected_buffer->flushing)
        {
            return -1;
        }

        while (selected_buffer->current_pos < MAX_BUFFER_SIZE && i < size)
        {
            selected_buffer->buffer[selected_buffer->current_pos++] = str[i++];
        }
        selected_buffer->require_flush = 1;
        dbg_dev->selected_buffer = dbg_dev->selected_buffer == 0 ? 1 : 0;

        size_t left = size - i;

        if (left > MAX_BUFFER_SIZE)
        {
            left = MAX_BUFFER_SIZE;
        }

        if (left)
        {
            memcpy(get_selected_buffer(dbg_dev).buffer, &str[i], left);
            get_selected_buffer(dbg_dev).current_pos = left;
        }
    }

    return 0;
}

static void __klog_klog_area_write(const uint8_t* str, size_t size)
{
    size_t str_start_cpy = 0;

    if (!klog_vstart)
    {
        return;
    }

    if (klog_vstart + klog_area_index + size > klog_vend)
    {
        size_t size_left = klog_vend - (klog_vstart + klog_area_index);
        memcpy(klog_vstart + klog_area_index, str, size_left);
        str_start_cpy = size_left;
        klog_area_index = 0;
        size -= str_start_cpy;
    }

    memcpy(klog_vstart + klog_area_index, &str[str_start_cpy], size);
    klog_area_index += size;
}

void klog_write(enum klog_logging_level log_level, const char *str, size_t size)
{
    __klog_klog_area_write((const uint8_t*) str, size);

    if (klog_default_handler.write != 0 && log_level >= klog_default_handler.log_level)
    {
        klog_default_handler.write(str, size);
    }

    if (klog_callbacks_vector.current_size <= 0)
    {
        return;
    }

    for (size_t i = 0; i < klog_callbacks_vector.current_size; i++)
    {
        klog_handler_t* handler = vector_get(&klog_callbacks_vector, i);
        if (log_level >= handler->log_level && handler->disabled == 0)
        {
            handler->write ? handler->write(str, size) : __klog_debug_device_write(handler, str, size);
        }
    }
}

int klog_set_default_handler(klog_write_callback write_callback, enum klog_logging_level log_level, enum klog_logging_method method)
{
    klog_default_handler.write = write_callback;
    klog_default_handler.log_method = method;
    klog_default_handler.log_level = log_level;

    return 0;
}

int klog_register_handler(klog_write_callback write_callback, enum klog_logging_level log_level, enum klog_logging_method method)
{
    klog_handler_t klog_callback = {
        .write = write_callback,
        .log_method = method,
        .log_level = log_level,
    };

    if (vector_push(&klog_callbacks_vector, &klog_callback) != 0)
    {
        return -1;
    }

    klog_handler_t* handler = vector_get(&klog_callbacks_vector, klog_callbacks_vector.current_size - 1);
    handler->index = klog_callbacks_vector.current_size - 1;
    handler->write((const char*) klog_vstart, klog_area_index);

    return 0;
}

int klog_register_fs_device(fs_device_t* device, enum klog_logging_level log_level, enum klog_logging_method method)
{
    klog_handler_t klog_callback = {
        .write = 0,
        .log_method = method,
        .log_level = log_level,
        .disabled = 1,
    };

    if (vector_push(&klog_callbacks_vector, &klog_callback) != 0)
    {
        return -1;
    }

    klog_handler_t* device_handler = vector_get(&klog_callbacks_vector, klog_callbacks_vector.current_size - 1);
    device_handler->index = klog_callbacks_vector.current_size - 1;

    device_handler->device = vmalloc(sizeof(klog_debug_device_t));

    if (!device_handler->device)
    {
        vector_pop(&klog_callbacks_vector, klog_callbacks_vector.current_size -1, &klog_callback);
        return -1;
    }

    kmutex_init(&device_handler->device->write_lock);
    device_handler->device->write_in_flight = 0;
    device_handler->device->device = device;
    device_handler->device->current_lba = 0;
    device_handler->device->start_lba = 0;
    device_handler->device->end_lba = device->gpt_partition->last_lba - device->gpt_partition->first_lba;
    device_handler->device->selected_buffer = 0;
    device_handler->device->buffers[0].buffer = vmalloc(MAX_BUFFER_SIZE);
    device_handler->device->buffers[0].current_pos = 0;
    device_handler->device->buffers[0].require_flush = 0;
    device_handler->device->buffers[0].flushing = 0;

    if (!device_handler->device->buffers[0].buffer)
    {
        vector_pop(&klog_callbacks_vector, klog_callbacks_vector.current_size -1, device_handler);
        vmfree(device_handler->device);
        return -1;
    }

    device_handler->device->buffers[1].buffer = vmalloc(MAX_BUFFER_SIZE);
    device_handler->device->buffers[1].current_pos = 0;
    device_handler->device->buffers[1].require_flush = 0;
    device_handler->device->buffers[1].flushing = 0;

    if (!device_handler->device->buffers[1].buffer)
    {
        vmfree(device_handler->device->buffers[0].buffer);
        vmfree(device_handler->device);
        vector_pop(&klog_callbacks_vector, klog_callbacks_vector.current_size -1, device_handler);
        return -1;
    }

    size_t copy_size = klog_area_index;

    if (klog_area_index % SECTOR_SIZE != 0)
    {
        copy_size += SECTOR_SIZE - (klog_area_index % SECTOR_SIZE);
    }

    device_handler->disabled = 0;
    if (__klog_safe_device_write(device_handler->device, klog_vstart, copy_size) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "klog : the boot log could not be replayed");
    }

    // if not equal to sector size
    if (klog_area_index % SECTOR_SIZE != 0)
    {
        // remove an lba else we would have that part of the logs twice.
        device_handler->device->current_lba = device_handler->device->current_lba == 0 ?
            0
                :
            device_handler->device->current_lba - 1;

        size_t cpy_log_start = klog_area_index - (klog_area_index % SECTOR_SIZE);
        size_t cpy_log_size = klog_area_index - cpy_log_start;
        memcpy(get_selected_buffer(device_handler->device).buffer, klog_vstart + cpy_log_start, cpy_log_size);
        get_selected_buffer(device_handler->device).current_pos = cpy_log_size;
    }

    return 0;
}

void klog_claim_buffer()
{
    extern uintptr_t klog_area;
    extern uintptr_t klog_area_end;

    klog_vstart = (uint8_t*) &klog_area;
    klog_vend = (uint8_t*) &klog_area_end;

    size_t size = klog_vend - klog_vstart;

    pmm_reserve((void*) V2P(klog_vstart), size);
    memset(klog_vstart, 0, size);
}

int init_klog()
{
    vector_init(&klog_callbacks_vector, sizeof(klog_handler_t), 5);
    kmutex_init(&klog_flush_lock);

    return 0;
}

static void __klog_flush(klog_handler_t* handler, size_t buffer_index)
{
    klog_debug_device_t* dbg_dev = handler->device;
    klog_device_buffer_t* selected_buffer = &dbg_dev->buffers[buffer_index];
    selected_buffer->flushing = 1;
    __klog_safe_device_write(dbg_dev, selected_buffer->buffer, selected_buffer->current_pos);

    selected_buffer->flushing = 0;
    selected_buffer->require_flush = 0;
    selected_buffer->current_pos = 0;
}

void klog_force_flush_buffers()
{
    kmutex_lock(&klog_flush_lock);
    for (size_t i = 0; i < klog_callbacks_vector.current_size; i++)
    {
        klog_handler_t* handler = vector_get(&klog_callbacks_vector, i);
        klog_debug_device_t* dbg_dev = handler->device;
        size_t unselected_buffer_index = dbg_dev->selected_buffer == 0 ? 1 : 0;
        if (dbg_dev->buffers[unselected_buffer_index].require_flush)
        {
            __klog_flush(handler, unselected_buffer_index);
        }
        __klog_flush(handler, dbg_dev->selected_buffer);
    }
}

void wormtongue()
{
    while (1)
    {
        for (size_t i = 0; i < klog_callbacks_vector.current_size; i++)
        {
            klog_handler_t* handler = vector_get(&klog_callbacks_vector, i);

            if (!handler->device)
            {
                continue;
            }

            for (size_t j = 0; j < 2; j++)
            {
                kmutex_lock(&klog_flush_lock);
                if (handler->device->buffers[j].require_flush)
                {
                    __klog_flush(handler, j);
                }
                kmutex_unlock(&klog_flush_lock);
            }
        }
    }

    // TODO : if this returns it crash the kernel
    while (1)
    {}
}