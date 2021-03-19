#pragma once

#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "balrog/input.h"

/**
 * @brief Initialize the keyboard
 * 
 */
void init_keyboard();

void keyboard_read(struct input_event* event);