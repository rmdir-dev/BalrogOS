#pragma once

#include <stdint.h>

void init_irq();

/**
 * @brief will send the require End Of Interrupt for the IRQs
 * 
 * @param id interrupt ID
 */
void irq_end(uint8_t id);