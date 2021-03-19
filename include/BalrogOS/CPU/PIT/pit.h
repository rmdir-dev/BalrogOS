#pragma once
#include <stdint.h>

/**
 * @brief initialize the programmable interval timer
 * 
 * @param frequency the timer frequency
 */
void init_pit(uint32_t frequency);