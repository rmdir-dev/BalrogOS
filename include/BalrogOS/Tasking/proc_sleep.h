//
// Created by rmdir on 10/21/23.
//

#ifndef BALROGOS_SLEEP_H
#define BALROGOS_SLEEP_H

#include <stdint.h>
#include <stddef.h>
#include "BalrogOS/Tasking/process.h"
#include "balrog/time/time.h"

/**
 * @brief
 * @param ms
 * @param proc
 */
void sleep(timespec* time, process* proc);

void remove_sleeper(process* proc);

#endif //BALROGOS_SLEEP_H
