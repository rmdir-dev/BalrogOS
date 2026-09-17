#pragma once

#include <stdint.h>

/*
Byte order conversions, both directions and both ends.

References :
endian : https://man7.org/linux/man-pages/man3/endian.3.html
*/

#define LITTLE_ENDIAN   __ORDER_LITTLE_ENDIAN__
#define BIG_ENDIAN      __ORDER_BIG_ENDIAN__
#define BYTE_ORDER      __BYTE_ORDER__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief host to big endian
 * 
 * @param host value in the host's own order
 * @return uint16_t the same value, most significant byte first
 */
uint16_t htobe16(uint16_t host);
uint32_t htobe32(uint32_t host);
uint64_t htobe64(uint64_t host);

/**
 * @brief host to little endian
 * 
 * @param host value in the host's own order
 * @return uint16_t the same value, least significant byte first
 */
uint16_t htole16(uint16_t host);
uint32_t htole32(uint32_t host);
uint64_t htole64(uint64_t host);

/**
 * @brief big endian to host
 * 
 * @param big value read most significant byte first
 * @return uint16_t the same value, in the host's own order
 */
uint16_t be16toh(uint16_t big);
uint32_t be32toh(uint32_t big);
uint64_t be64toh(uint64_t big);

/**
 * @brief little endian to host
 * 
 * @param little value read least significant byte first
 * @return uint16_t the same value, in the host's own order
 */
uint16_t le16toh(uint16_t little);
uint32_t le32toh(uint32_t little);
uint64_t le64toh(uint64_t little);

#ifdef __cplusplus
}
#endif
