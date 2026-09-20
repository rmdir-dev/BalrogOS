#include "balrog_os/cpu/rtc/rtc.h"

#include "balrog_os/cpu/pit/pit.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/debug/debug_output.h"

#define RTC_MAX_WAIT        1000000

/**
 * Bynary Coded Decimal each number from 0 to 9 are represented into 1 group of
 * 4 bits (called a nibble).
 *
 * so lower number is value & 0x0f and the higher is (value >> 4) * 10
 */
#define BCD_TO_BIN(value)   (((value) & 0x0F) + (((value) >> 4) * 10))

static time_t __rtc_boot_epoch = 0;

static uint8_t __rtc_read(uint8_t reg)
{
    out_byte(CMOS_ADDRESS, reg);

    return in_byte(CMOS_DATA);
}

static int __rtc_wait_idle()
{
    // without a CMOS a CPU answer 0xff on all ports, so the update flag would
    // never clear so we bound this wait
    for (int wait = 0; wait < RTC_MAX_WAIT; wait++)
    {
        if (!(__rtc_read(RTC_STATUS_A) & RTC_UPDATE_IN_PROGRESS))
        {
            return 0;
        }
    }

    return -1;
}

static int __rtc_decode(rtc_time_t* time)
{
    uint8_t status_b = __rtc_read(RTC_STATUS_B);
    // if not RTC_24_HOUR and pm flag (0x80)
    uint8_t pm = !(status_b & RTC_24_HOUR) && (time->hour & 0x80);

    // extract hour without the pm flag (everything bellow 0x80)
    time->hour &= 0x7F;

    // if type BCD, the we need to decode it.
    if(!(status_b & RTC_BINARY))
    {
        time->second = BCD_TO_BIN(time->second);
        time->minute = BCD_TO_BIN(time->minute);
        time->hour   = BCD_TO_BIN(time->hour);
        time->day    = BCD_TO_BIN(time->day);
        time->month  = BCD_TO_BIN(time->month);
        time->year   = BCD_TO_BIN(time->year);
    }

    // transform pm to 24h format.
    if (pm)
    {
        time->hour = (time->hour % 12) + 12;
    // transform am to 24h format (make sure am does not overflow).
    } else if (!(status_b & RTC_24_HOUR))
    {
        time->hour = time->hour % 12;
    }

    // adapt time either.
    // TODO : Fix this before 2070 ! else this become a time machine.
    time->year += time->year < 70 ? 2000 : 1900;

    // if any entry is invalid
    if(time->month < 1 || time->month > 12 || time->day < 1 || time->day > 31
        || time->hour > 23 || time->minute > 59 || time->second > 60)
    {
        return -1;
    }

    return 0;
}

/*  days_from_civil, from Howard Hinnant's chrono papers. Era based, no loop,
    exact on leap years including the 100 and 400 year rules.
    http://howardhinnant.github.io/date_algorithms.html  */
static long __rtc_days_from_civil(long year, unsigned month, unsigned day)
{
    year -= month <= 2;

    const long era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned) (year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097L + (long) doe - 719468L;
}

static time_t __rtc_to_epoch(rtc_time_t* time)
{
    long days = __rtc_days_from_civil(time->year, time->month, time->day);

    return (time_t) (days * 86400L + time->hour * 3600L + time->minute * 60L + time->second);
}

static int __rtc_read_raw(rtc_time_t* time)
{
    if(__rtc_wait_idle() != 0)
    {
        return -1;
    }

    time->second = __rtc_read(RTC_SECOND);
    time->minute = __rtc_read(RTC_MINUTE);
    time->hour   = __rtc_read(RTC_HOUR);
    time->day    = __rtc_read(RTC_DAY);
    time->month  = __rtc_read(RTC_MONTH);
    time->year   = __rtc_read(RTC_YEAR);

    return 0;
}

/**
 * @brief it can happen that a clock tick between byte reads so we read multiple time
 *        and only take a pair that agree.
 *
 * @param time
 * @return
 */
static int __rtc_read_stable(rtc_time_t* time)
{
    rtc_time_t previous = {};

    if(__rtc_read_raw(&previous) != 0)
    {
        return -1;
    }

    for(int try = 0; try < 16; try++)
    {
        if(__rtc_read_raw(time) != 0)
        {
            return -1;
        }

        if(time->second == previous.second && time->minute == previous.minute
            && time->hour == previous.hour && time->day == previous.day
            && time->month == previous.month && time->year == previous.year)
        {
            return 0;
        }

        previous = *time;
    }

    return -1;
}

time_t rtc_boot_epoch()
{
    return __rtc_boot_epoch;
}

time_t get_unix_time()
{
    timespec now = {0, 0};
    get_current_time(&now);

    return __rtc_boot_epoch + now.sec;
}

int init_rtc()
{
    rtc_time_t now = {};

    if(__rtc_read_stable(&now) != 0 || __rtc_decode(&now) != 0)
    {
        kernel_debug_output(KDB_LVL_WARNING, "rtc : no usable cmos clock, timestamps stay relative");
        return -1;
    }

    __rtc_boot_epoch = __rtc_to_epoch(&now);

    kernel_debug_output(KDB_LVL_INFO, "rtc : %d-%d-%d %d:%d:%d, epoch %d",
        now.year, now.month, now.day, now.hour, now.minute, now.second, __rtc_boot_epoch);

    return 0;
}
