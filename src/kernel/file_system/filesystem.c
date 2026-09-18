#include "balrog_os/file_system/ext2/ext2_cache/ext2_cache.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/file_system/fs_devices.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/disk/ahci/ahci.h"
#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/drivers/disk/ramfs/ramfs.h"
#include "balrog_os/file_system/vfs/vfs.h"
#include "balrog_os/tasking/elf/elf.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include <string.h>

#include "errno.h"
#include "balrog_os/file_system/gpt/gpt.h"
#include "klib/io/kprint.h"

fs_device_t boot_dev;
static vfs_root_t vfs_root;
list_t devices;

static const size_t fs_devices_lookout_len = 3;

extern int ata_scan_devices();
extern int ahci_scan_devices();
extern int xhci_scan_devices();

static int (*fs_devices_lookout[])() =
{
    [0] &ahci_scan_devices,
    [1] &ata_scan_devices,
    [2] &xhci_scan_devices,
};

/*  same order as fs_devices_lookout above  */
static const char* fs_devices_lookout_name[] = {
    "ahci",
    "ata",
    "xhci",
};

int fs_get_file(const char* name, fs_file* file, fs_fd* fd)
{
    size_t len = strlen(name);
    char* fname = vmalloc(len + 1);
    memcpy(fname, name, len);
    fname[len] = 0;

    int ret = fs_open(fname, fd);

    vmfree(fname);

    if(ret != 0)
    {
        return -1;
    }

    fs_file* tmp = fs_cache_get_file(fd->ftable_idx);

    if(!tmp)
    {
        return -1;
    }

    *file = *tmp;

    return 0;
}

static int __fs_device_partuuid_lookup(list_node_t* node, const void* key)
{
    fs_device_t* device = node->value;

    if (!device->gpt_partition)
    {
        return -1;
    }

    char uuid[GPT_GUID_TEXT_LEN];
    gpt_read_guid(device->gpt_partition->unique_guid, uuid);
    int ret = strcmp(uuid, (const char*) key);

    return ret;
}

int fs_mount(const char* mount_path, const char* name_or_uuid)
{
    list_node_t* node;
    size_t len = strlen(name_or_uuid);

    // must pass /dev/ or uuid
    if (len < 5)
    {
        return ENOENT;
    }

    if (memcmp(name_or_uuid, "/dev/", 5) == 0)
    {
        node = list_str_lookup(&devices,  (const char*) name_or_uuid + 5);
    } else
    {
        node = list_custom_lookup(&devices, name_or_uuid, &__fs_device_partuuid_lookup);
    }

    if (!node)
    {
        return ENODEV;
    }

    fs_device_t* device = (fs_device_t*) node->value;
    // TODO search device.
    return vfs_root.mount(&vfs_root, mount_path, device);
}

int fs_open(char* name, fs_fd* fd)
{
    return vfs_root.open(&vfs_root, name, fd);
}

int fs_read(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    return vfs_root.read(&vfs_root, buffer, len, fd);
}

int fs_write(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    return vfs_root.write(&vfs_root, buffer, len, fd);
}

int fs_touch(char* filename)
{
    return fs_create(filename, 0);
}

int fs_create(char* filename, uint64_t size)
{
    return vfs_root.create(&vfs_root, filename, size);
}

int fs_mkdir(char* dirname)
{
    return vfs_root.mkdir(&vfs_root, dirname);
}

int fs_unlink(char* filename)
{
    return vfs_root.unlink(&vfs_root, filename);
}

int fs_rmdir(char* dirname)
{
    return vfs_root.rmdir(&vfs_root, dirname);
}

int fs_close(fs_fd* fd)
{
    return vfs_root.close(&vfs_root, fd);
}

int fs_fstat(fs_fd* fd, fs_file_stat* stat)
{
    return vfs_root.stat(&vfs_root, fd, stat);
}

void fs_device_init(fs_device_t* device)
{
    kmutex_init(&device->lock);
    device->partition_table = NULL;
    device->gpt_header = NULL;
    device->gpt_partition = NULL;
}

void fs_add_device(fs_device_t* device)
{
    kernel_debug_output(KDB_LVL_INFO, "file system : adding device uuid: %d", device->unique_id);
    list_insert(&devices, (size_t) device->name, device);
    uint8_t part_count = 1;

    if (gpt_init(device) == 0)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "found gpt table");
        for (uint32_t i = 0; i < device->gpt_header->entry_count; i++)
        {
            if (part_count > 99)
            {
                kernel_debug_output(KDB_LVL_ERROR, "partition found > 99 entries");
                break;
            }
            fs_device_t* part_device = vmalloc(sizeof(fs_device_t));
            memcpy(part_device, device, sizeof(fs_device_t));
            kmutex_init(&part_device->lock);

            if (gpt_find_by_index(part_device, i) != 0)
            {
                vmfree(part_device);
                continue;
            }

            kernel_debug_output(KDB_LVL_INFO, "fs : partition found & initialized");
            part_device->partition_table = NULL;

            // GPT values
            part_device->gpt_header = vmalloc(sizeof(gpt_header_t));
            memcpy(part_device->gpt_header, device->gpt_header, sizeof(gpt_header_t));

            // Name
            size_t name_len = strlen(device->name);
            part_device->name = vmalloc(name_len + 3); // name_len + nullbyte + 2 byte buffer
            memcpy(part_device->name, device->name, name_len);
            size_t shift = 0;
            if (part_count >= 10)
            {
                shift++;
                part_device->name[name_len] = '0' + ((part_count / 10) % 10);
            }
            part_device->name[name_len + shift++] = '0' + part_count % 10;
            part_device->name[name_len + shift] = 0;

            list_insert(&devices, (size_t) part_device->name, part_device);

            part_count++;
        }
    }
}

static int __scan_devices_and_initramdisk()
{
    // ramdisk first as we will use it to load the real root disk later on
    if (ramdisk_get_boot_device(&boot_dev))
    {
        kernel_debug_output(KDB_LVL_ERROR, "file system : none of the %d drivers found a boot device", fs_devices_lookout_len);
        return -1;
    }

    if (ext2_probe(&boot_dev) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "file system : %s has no ext2, trying the next", boot_dev.name);
        return -1;
    }

    for(size_t i = 0; i < fs_devices_lookout_len; i++)
    {
        const char* name = (i < fs_devices_lookout_len) ? fs_devices_lookout_name[i] : "?";

        kernel_debug_output(KDB_LVL_INFO, "file system : asking %s for a boot device", name);

        fs_devices_lookout[i]();
    }

    return 0;
}

uint64_t get_first_lba(fs_device_t* device)
{
    uint64_t first_lba = device->first_lba;
    if (device->gpt_partition)
    {
        first_lba = device->gpt_partition->first_lba;
    }
    return first_lba;
}

int init_file_system()
{
    list_init(&devices);
    ramdisk_init((void*)P2V(RAMFS_PHYS), RAMFS_SIZE);

    kmutex_init(&boot_dev.lock);
    kmutex_lock(&boot_dev.lock);

    /*
    Initialize ext2 cache datastructures.
    */
    ext2_cache_init();

    if(__scan_devices_and_initramdisk() != 0)
    {
        kmutex_unlock(&boot_dev.lock);
        KERNEL_LOG_FAIL("file system : No suitable drive found!");
        while(1){}
    }

    vfs_init(&vfs_root, &boot_dev);

    kmutex_unlock(&boot_dev.lock);

    return 0;
}