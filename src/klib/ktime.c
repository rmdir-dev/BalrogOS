#include "klib/ktime.h"

#include "balrog_os/cpu/rtc/rtc.h"

time_t ktime(time_t* second)
{
    time_t now = get_unix_time();

    if(second)
    {
        *second = now;
    }

    return now;
}
