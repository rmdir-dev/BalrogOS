#pragma once

#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog/input.h"

#define KEYBOARD_QUEUE_SIZE     64

/**
 * @brief Initialize the keyboard
 * 
 */
int init_keyboard();

int keyboard_read(struct input_event* event);