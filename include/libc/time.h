#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long time_t;

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