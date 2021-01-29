#pragma once

#include <stdint.h>

void init_irq();

/**
 * @brief will send the require End Of Interrupt for the IRQs
 * 
 * @param id interrupt ID
 */
void irq_end(uint8_t id);

void irq_pic_toggle_mask_bit(uint8_t irq_id);