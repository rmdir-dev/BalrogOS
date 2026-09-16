#pragma once

#include <stdint.h>
#include "balrog_os/cpu/pit/pit.h"

/*
Advanced Programmable Interrupt Controller, the timer part
Documentation : 
    APIC timer : https://wiki.osdev.org/APIC_timer
*/

/**
 * @brief
 *
 * @param hz
 * @param event
 * @return
 */
int lapic_timer_init(uint32_t hz, pit_event event);

/**
 * @brief end of interrupt
 */
void lapic_eoi();
