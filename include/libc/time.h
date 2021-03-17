#pragma once

#include "balrog/time/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the time in second since 1st january 1970
 * 
 * @param second 
 * @return time_t 
 */
time_t time(time_t* second);

#ifdef __cplusplus
}
#endif