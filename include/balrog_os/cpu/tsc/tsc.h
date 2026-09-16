#pragma once

#include <stdint.h>

/*
Time Stamp Counter
Documentation : 
    TSC : https://wiki.osdev.org/TSC
*/

/**
 * @brief Read the time stamp counter.
 *
 * @return uint64_t the cycle count since the cpu came up
 */
uint64_t tsc_read();

/**
 * @brief How many tsc cycles go by in 100ns.
 *
 * @return uint64_t the ratio, never 0
 */
uint64_t tsc_per_100ns();

/**
 * @brief Busy wait, counted on the tsc.
 *
 * @param units how long to wait, in units of 100ns
 */
void tsc_wait_100ns(uint64_t units);
