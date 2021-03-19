#pragma once

#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "klib/IO/kprint.h"

/**
 * @brief Initialize the keyboard
 * 
 */
void init_keyboard();

void keyboard_read(struct input_event* event);