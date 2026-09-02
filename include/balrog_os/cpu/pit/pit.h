#pragma once
#include <stdint.h>
#include "balrog/time/time.h"

typedef void (*pit_event)(size_t tick, uint16_t ms);

/**
 * @brief initialize the programmable interval timer
 *
 * @param frequency the timer frequency
 */
void init_pit(pit_event scheduler);

void get_relative_time(timespec* time, timespec* relative_time);

/**
 * @brief compare if the time specified is before the current time
 *
 * @param time
 * @return
 */
int pit_compare(timespec* time);
