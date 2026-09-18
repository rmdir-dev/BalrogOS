#pragma once

#include <stdint.h>

void init_irq();

/**
 * @brief will send the require End Of Interrupt for the IRQs
 * 
 * @param id interrupt ID
 */
void irq_end(uint8_t id);

/**
 * @brief Set the bit of an IRQ into the master/slave PIC mask
 * 
 * @param irq_id interrupt vector, INT_IRQ_0 based
 */
void irq_pic_mask(uint8_t irq_id);

/**
 * @brief Clear the bit of an IRQ into the master/slave PIC mask
 * 
 * @param irq_id interrupt vector, INT_IRQ_0 based
 */
void irq_pic_unmask(uint8_t irq_id);