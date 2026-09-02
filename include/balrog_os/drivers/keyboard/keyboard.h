#pragma once

#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog/input.h"

/**
 * @brief Initialize the keyboard
 * 
 */
int init_keyboard();

void keyboard_read(struct input_event* event);