#include <arpa/inet.h>
#include <endian.h>

/* user the compiler BYTE_ORDER to know which way to switch bytes */
#if BYTE_ORDER == LITTLE_ENDIAN
    #define TO_BIG(bits, value)     __builtin_bswap##bits(value)
    #define TO_LITTLE(bits, value)  (value)
#else
    #define TO_BIG(bits, value)     (value)
    #define TO_LITTLE(bits, value)  __builtin_bswap##bits(value)
#endif

uint16_t htobe16(uint16_t host)
{
    return TO_BIG(16, host);
}

uint32_t htobe32(uint32_t host)
{
    return TO_BIG(32, host);
}

uint64_t htobe64(uint64_t host)
{
    return TO_BIG(64, host);
}

uint16_t htole16(uint16_t host)
{
    return TO_LITTLE(16, host);
}

uint32_t htole32(uint32_t host)
{
    return TO_LITTLE(32, host);
}

uint64_t htole64(uint64_t host)
{
    return TO_LITTLE(64, host);
}

uint16_t be16toh(uint16_t big)
{
    return TO_BIG(16, big);
}

uint32_t be32toh(uint32_t big)
{
    return TO_BIG(32, big);
}

uint64_t be64toh(uint64_t big)
{
    return TO_BIG(64, big);
}

uint16_t le16toh(uint16_t little)
{
    return TO_LITTLE(16, little);
}

uint32_t le32toh(uint32_t little)
{
    return TO_LITTLE(32, little);
}

uint64_t le64toh(uint64_t little)
{
    return TO_LITTLE(64, little);
}

/*  network order is big endian  */
uint16_t htons(uint16_t host)
{
    return TO_BIG(16, host);
}

uint32_t htonl(uint32_t host)
{
    return TO_BIG(32, host);
}

uint16_t ntohs(uint16_t net)
{
    return TO_BIG(16, net);
}

uint32_t ntohl(uint32_t net)
{
    return TO_BIG(32, net);
}
