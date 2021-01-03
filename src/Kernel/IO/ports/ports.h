#pragma once

#include <stdint.h>

void out_byte(uint16_t port, uint8_t value);
uint8_t in_byte(uint16_t port);

void out_word(uint16_t port, uint16_t value);
uint16_t in_word(uint16_t port);
