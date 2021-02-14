#pragma once

#include <stdint.h>

/**
 * @brief send one byte to a port
 * 
 * @param port port address
 * @param value value (in byte) to send
 */
void out_byte(uint16_t port, uint8_t value);

/**
 * @brief get the value (in byte) from a port
 * 
 * @param port port address
 * @return uint8_t value send by the port
 */
uint8_t in_byte(uint16_t port);

/**
 * @brief send a word to a port
 * 
 * @param port port address
 * @param value value (in word) to send
 */
void out_word(uint16_t port, uint16_t value);

/**
 * @brief get a value from a port
 * 
 * @param port port address
 * @return uint16_t value send by the port
 */
uint16_t in_word(uint16_t port);
