#include "balrog/uuid/uuid.h"

/*
The first three fields of a uuid are byte swapped and the last two are not,
which is what this encodes. ext2 does NOT store its uuid this way.
*/
static const uint8_t __gpt_uuid_order[UUID_LEN] =
{
    // big endian
    3, 2, 1, 0,
    // big endian
    5, 4,
    // big endian
    7, 6,
    // data -> little endia
    8, 9,
    10, 11, 12, 13, 14, 15
};

static const char __uuid_hex[] = "0123456789ABCDEF";

static int __uuid_hex_value(char c)
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

void uuid_to_str(const uint8_t* uuid, char* out)
{
    uint64_t o = 0;

    for(uint64_t i = 0; i < UUID_LEN; i++)
    {
        /*  the dashes sit after the 4th, 6th, 8th and 10th byte  */
        if(i == 4 || i == 6 || i == 8 || i == 10)
        {
            out[o++] = '-';
        }

        uint8_t byte = uuid[__gpt_uuid_order[i]];
        out[o++] = __uuid_hex[byte >> 4];
        out[o++] = __uuid_hex[byte & 0x0F];
    }

    out[o] = 0;
}

int str_to_uuid(uint8_t* uuid, const char* in)
{
    uint64_t t = 0;

    for(uint64_t i = 0; i < UUID_LEN; i++)
    {
        if(i == 4 || i == 6 || i == 8 || i == 10)
        {
            if(in[t] != '-')
            {
                return -1;
            }

            t++;
        }

        int high = __uuid_hex_value(in[t]);
        int low = __uuid_hex_value(in[t + 1]);

        if(high < 0 || low < 0)
        {
            return -1;
        }

        uuid[__gpt_uuid_order[i]] = (uint8_t) ((high << 4) | low);
        t += 2;
    }

    /* must be 36 long else it is not a uuid */
    return (in[t] == 0) ? 0 : -1;
}