#pragma once

#include <stdint.h>
#include "balrog/time/time.h"

/*
Real Time Clock, the battery backed clock sitting in the CMOS.
Documentation :
    RTC : https://wiki.osdev.org/RTC
    CMOS : https://wiki.osdev.org/CMOS
*/

#define CMOS_ADDRESS            0x70
#define CMOS_DATA               0x71

#define RTC_SECOND              0x00
#define RTC_MINUTE              0x02
#define RTC_HOUR                0x04
#define RTC_DAY                 0x07
#define RTC_MONTH               0x08
#define RTC_YEAR                0x09
#define RTC_STATUS_A            0x0A
#define RTC_STATUS_B            0x0B

#define RTC_UPDATE_IN_PROGRESS  0x80    // status A, the clock is mid update
#define RTC_24_HOUR             0x02    // status B, else bit 7 of hour means pm
#define RTC_BINARY              0x04    // status B, else every field is bcd

typedef struct __rtc_time_t
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

/**
 * @brief the unix epoch read at boot, 0 if the clock was unusable
 *
 * @return time_t
 */
time_t rtc_boot_epoch();

/**
 * @brief boot epoch plus the uptime
 *
 * @return time_t
 */
time_t get_unix_time();

int init_rtc();
