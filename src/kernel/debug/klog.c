#include "balrog_os/debug/klog.h"

#include "string.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/pmm.h"
#include "klib/data_structure/list.h"
#include "klib/data_structure/vector.h"

typedef struct __klog_callback_t
{
    klog_write_callback write;
    enum klog_logging_method method;
} klog_handler_t;

extern uintptr_t* klog_area;
extern uintptr_t* klog_area_end;
klog_handler_t default_handler;
size_t klog_area_index = 0;

vector_t klog_callbacks_vector = {
    .current_size = 0,
    .data = 0,
    .__data_size = 0,
    .__max_size = 0,
    .__mem_size = 0,
    .__growth = 0
};

int enabled = 0;

void klog_write(const char *str, size_t size)
{
    if (default_handler.write != 0)
    {
        default_handler.write(str, size);
    }

    // TODO : log into klog area and purge the logs from time to time.

    if (klog_callbacks_vector.current_size <= 0)
    {
        return;
    }

    for (size_t i = 0; i < klog_callbacks_vector.current_size; i++)
    {
        klog_handler_t* handler = vector_get(&klog_callbacks_vector, i);
        handler->write(str, size);
    }
}

int klog_set_default_handler(klog_write_callback write_callback, enum klog_logging_method method)
{
    default_handler.write = write_callback;
    default_handler.method = method;

    return 0;
}

int klog_register_handler(klog_write_callback write_callback, enum klog_logging_method method)
{
    klog_handler_t klog_callback = {
        .write = write_callback,
        .method = method,
    };

    return vector_push(&klog_callbacks_vector, &klog_callback) == 0 ? 0 : -1;
}

void klog_claim_buffer()
{
    size_t size = (uintptr_t)&klog_area_end - (uintptr_t)&klog_area;
    pmm_reserve((void*) V2P(&klog_area), size);
    memset(&klog_area, 0, size);
}

int init_klog()
{
    vector_init(&klog_callbacks_vector, sizeof(klog_handler_t), 5);

    return 0;
}