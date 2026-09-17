#pragma once

#include <stdint.h>

/*
The four POSIX byte order calls.

Network order is big endian, so on this AMD64 all four swap.

References :
htonl : https://pubs.opengroup.org/onlinepubs/9699919799/functions/htonl.html
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief host to network, 16 bits
 * 
 * @param host value in the host's own order
 * @return uint16_t the same value in network order
 */
uint16_t htons(uint16_t host);

/**
 * @brief host to network, 32 bits
 * 
 * @param host value in the host's own order
 * @return uint32_t the same value in network order
 */
uint32_t htonl(uint32_t host);

/**
 * @brief network to host, 16 bits
 * 
 * @param net value in network order
 * @return uint16_t the same value in the host's own order
 */
uint16_t ntohs(uint16_t net);

/**
 * @brief network to host, 32 bits
 * 
 * @param net value in network order
 * @return uint32_t the same value in the host's own order
 */
uint32_t ntohl(uint32_t net);

#ifdef __cplusplus
}
#endif
