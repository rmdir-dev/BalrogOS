#pragma once

#include <stdint.h>

/**
 * @brief Initialize the serial
 *
 * @return status 0 ok, -1 error
 */
int serial_init();

/**
 * @brief Read a single char from serial
 *
 * @return read char
 */
char serial_read_char();

/**
 * @brief Read a buffer from serial
 *
 * @param buffer
 * @param size
 */
void serial_read_buffer(char* buffer, size_t size);

/**
 * @brief send a single byte to serial
 *
 * @param c
 */
void serial_put_char(char c);

/**
 * @brief read bytes from serial.
 *
 * @param s
 */
void serial_write(const char *s);