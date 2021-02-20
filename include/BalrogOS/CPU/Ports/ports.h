#pragma once

#include <stdint.h>

/**
 * @brief send one byte to a port
 * 
 * @param port port address
 * @param value value (in byte) to send
 */
static inline void out_byte(uint16_t port, uint8_t value)
{
    /*
    * outb = out byte
    * "a" = set value into rax register
    * "Nd" = N constraint d rdx register
    */
    asm volatile("outb %0, %1": : "a"(value), "Nd"(port));
}

/**
 * @brief get the value (in byte) from a port
 * 
 * @param port port address
 * @return uint8_t value send by the port
 */
static inline uint8_t in_byte(uint16_t port)
{
    uint8_t ret;
    /*
    * inb = in byte
    * "=a" = output constraint to rax, and set rax into ret
    * "Nd" = N constraint d rdx register
    */
    asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

/**
 * @brief send a word to a port
 * 
 * @param port port address
 * @param value value (in word) to send
 */
static inline void out_word(uint16_t port, uint16_t value)
{
    /*
    * outw = out word
    * "a" = set value into rax register
    * "Nd" = N constraint d rdx register
    */
    asm volatile("outw %0, %1": : "a"(value), "Nd"(port));
}

/**
 * @brief get a value from a port
 * 
 * @param port port address
 * @return uint16_t value send by the port
 */
static inline uint16_t in_word(uint16_t port)
{
    uint16_t ret;
    /*
    * inw = in word
    * "=a" = output constraint to rax, and set rax into ret
    * "Nd" = N constraint d rdx register
    */
    asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}
