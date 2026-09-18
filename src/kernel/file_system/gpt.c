#include "balrog_os/file_system/gpt/gpt.h"

#include "string.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/file_system/gpt/gpt_guid.h"
#include "balrog_os/file_system/gpt/gpt_struct.h"
#include "balrog_os/memory/kheap.h"


int __gpt_read_header(fs_device_t *dev)
{
    kernel_debug_output(KDB_LVL_INFO, "gpt : read header");
    uint8_t* buffer = vmalloc(ATA_SECTOR_SIZE);

    if (!buffer)
    {
        kernel_debug_output(KDB_LVL_ERROR, "gpt : no memory for reading header");
        return -1;
    }

    if(dev->read(dev, buffer, GPT_HEADER_LBA, 1) != 0){
        kernel_debug_output(KDB_LVL_ERROR, "gpt : failed to read header");
        vmfree(buffer);
        return -1;
    }

    dev->gpt_header = vmalloc(sizeof(gpt_header_t));

    if (!dev->gpt_header)
    {
        vmfree(buffer);
        kernel_debug_output(KDB_LVL_ERROR, "gpt : no memory for reading header");
        return -1;
    }

    memcpy(dev->gpt_header, buffer, sizeof(gpt_header_t));
    vmfree(buffer);

    if (memcmp(dev->gpt_header->signature, GPT_SIGNATURE, GPT_SIGNATURE_LEN) != 0)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "gpt : no signature at lba %d, not a gpt disk", GPT_HEADER_LBA);
        vmfree(dev->gpt_header);
        dev->gpt_header = NULL;
        return -1;
    }

    if (
            dev->gpt_header->entry_size < sizeof(gpt_entry_t)
            || dev->gpt_header->entry_count == 0
            || dev->gpt_header->entry_count > GPT_MAX_ENTRIES
        )
    {
        kernel_debug_output(KDB_LVL_ERROR, "gpt : %d entries of %d bytes, refusing to walk that",
            dev->gpt_header->entry_count, dev->gpt_header->entry_size);
        vmfree(dev->gpt_header);
        dev->gpt_header = NULL;
        return -1;
    }

    kernel_debug_output(KDB_LVL_INFO, "gpt : header read!");
    return 0;
}

static void __gpt_get_partition(fs_device_t *dev, const gpt_entry_t* entry, uint32_t index)
{
    kernel_debug_output(KDB_LVL_INFO, "gtp : retrieving partition %d", index);
    dev->gpt_partition = vmalloc(sizeof(gpt_partition_t));
    memcpy(dev->gpt_partition->type_guid, entry->type_guid, GPT_GUID_LEN);
    memcpy(dev->gpt_partition->unique_guid, entry->unique_guid, GPT_GUID_LEN);

    dev->gpt_partition->first_lba = entry->first_lba;
    dev->gpt_partition->last_lba = entry->last_lba;
    dev->gpt_partition->index = index;

    size_t i = 0;
    for (i = 0; i < GPT_NAME_LEN - 1 && entry->name[i]; i++)
    {
        dev->gpt_partition->name[i] = (entry->name[i] < 0x100) ? (char) entry->name[i] : '?';
    }

    dev->gpt_partition->name[i] = 0;
}

#define GPT_LOOKUP_TYPE_GUID        0
#define GPT_LOOKUP_TYPE_INDEX       1

static int __gpt_lookup(fs_device_t *dev, int lookup_type, const void* key)
{
    kernel_debug_output(KDB_LVL_INFO, "gpt : looking up type %d", lookup_type);
    int found = -1;

    // If search by index, then jump to the index.
    for(uint32_t i = lookup_type == GPT_LOOKUP_TYPE_INDEX ? *((uint32_t*) key) : 0; i < dev->gpt_header->entry_count; i++)
    {
        const gpt_entry_t* entry = (const gpt_entry_t*) (dev->partition_table + (uint64_t) i * dev->gpt_header->entry_size);

        /*  an empty slot can sit in the middle of the table, so this is a skip
            and not a stop.  */
        static const uint8_t empty[GPT_GUID_LEN] = {};

        if(memcmp(entry->type_guid, empty, GPT_GUID_LEN) == 0)
        {
            continue;
        }

        int lookup = lookup_type == GPT_LOOKUP_TYPE_GUID ?
            memcmp(entry->unique_guid, key, GPT_GUID_LEN) == 0
                :
            i == *(const uint32_t*) key;
        if(lookup)
        {

            kernel_debug_output(KDB_LVL_INFO, "gpt : found gpt entry at lba %d", i);
            __gpt_get_partition(dev, entry, i);
            found = 0;
            break;
        }
    }

    return found;
}

int gpt_find_by_index(fs_device_t *dev, uint32_t index)
{
    return __gpt_lookup(dev, GPT_LOOKUP_TYPE_INDEX, &index);
}

int gpt_find_by_guid(fs_device_t *dev,  const uint8_t* guid)
{
    return __gpt_lookup(dev, GPT_LOOKUP_TYPE_GUID, guid);
}

static const char __gpt_hex[] = "0123456789ABCDEF";

void gpt_read_guid(const uint8_t* guid, char* out)
{
    static const uint8_t order[GPT_GUID_LEN] =
    {
        3, 2, 1, 0,
        5, 4,
        7, 6,
        8, 9,
        10, 11, 12, 13, 14, 15
    };

    size_t o = 0;

    for(size_t i = 0; i < GPT_GUID_LEN; i++)
    {
        /*  the dashes sit after the 4th, 6th, 8th and 10th byte  */
        if(i == 4 || i == 6 || i == 8 || i == 10)
        {
            out[o++] = '-';
        }

        uint8_t byte = guid[order[i]];
        out[o++] = __gpt_hex[byte >> 4];
        out[o++] = __gpt_hex[byte & 0x0F];
    }

    out[o] = 0;
}

static int __str_hex_to_int(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }

    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    return -1;
}

int gpt_write_guid(uint8_t* guid, const char* in)
{
    static const uint8_t order[GPT_GUID_LEN] =
    {
        3, 2, 1, 0,
        5, 4,
        7, 6,
        8, 9,
        10, 11, 12, 13, 14, 15
    };

    size_t t = 0;

    for(size_t i = 0; i < GPT_GUID_LEN; i++)
    {
        if(i == 4 || i == 6 || i == 8 || i == 10)
        {
            if(in[t] != '-')
            {
                return -1;
            }

            t++;
        }

        int high = __str_hex_to_int(in[t]);
        int low = __str_hex_to_int(in[t + 1]);

        if(high < 0 || low < 0)
        {
            return -1;
        }

        guid[order[i]] = (uint8_t) ((high << 4) | low);
        t += 2;
    }

    // 36 characters and nothing after them.
    return (in[t] == 0) ? 0 : -1;
}

int gpt_init(fs_device_t *dev)
{
    kernel_debug_output(KDB_LVL_INFO, "initializing gpt for device %d", dev->unique_id);
    if(__gpt_read_header(dev) != 0)
    {
        return -1;
    }

    uint64_t bytes = (uint64_t) dev->gpt_header->entry_count * dev->gpt_header->entry_size;
    uint64_t sectors = (bytes + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
    dev->partition_table = vmalloc(sectors * ATA_SECTOR_SIZE);

    if (!dev->partition_table)
    {
        vmfree(dev->gpt_header);
        dev->gpt_header = NULL;
        return -1;
    }

    if (dev->read(dev, dev->partition_table, dev->gpt_header->entry_lba, sectors) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "gpt : cannot read the table at lba %d",
                        dev->gpt_header->entry_lba);
        vmfree(dev->partition_table);
        dev->partition_table = NULL;
        vmfree(dev->gpt_header);
        dev->gpt_header = NULL;
        return -1;
    }

    return 0;
}