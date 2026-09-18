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

int fs_mount(const char* path, uint8_t* uuid)
{
    // TODO search disk
    fs_device_t* device = vmalloc(sizeof(fs_device_t));
    return vfs_root.mount(&vfs_root, path, device);
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

void fs_add_device(fs_device_t* device)
{
    kernel_debug_output(KDB_LVL_INFO, "file system : adding device uuid: %d", device->unique_id);
    list_insert(&devices, (int) device->unique_id, device);
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