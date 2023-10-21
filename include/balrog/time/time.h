#pragma once

#include <stdint.h>

typedef unsigned long time_t;

typedef struct timespec_t {
    time_t sec;      // seconds
    uint16_t msec;   // ms
} timespec;