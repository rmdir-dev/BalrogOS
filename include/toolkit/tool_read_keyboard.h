//
// Created by rmdir on 10/20/23.
//

#ifndef BALROGOS_TOOL_READ_KEYBOARD_H
#define BALROGOS_TOOL_READ_KEYBOARD_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>

/**
 * @brief
 * @param input
 * @param buffer
 * @param buf_idx
 * @param manage_ctrl
 * @return
 */
int process_input(struct input_event* input, char* buffer, uint32_t* buf_idx, uint8_t print,
        int (*manage_special_keys)(uint16_t special, uint16_t pressed, uint8_t* ky));

extern char* cursor;
extern char* cursor_color;

typedef struct _user_info_t {
    char* username;
    char* home_dir;
    char* shell;
    uint32_t uid;
    uint32_t gid;
} user_info_t;

void get_user_info(user_info_t* info);

#endif //BALROGOS_TOOL_READ_KEYBOARD_H
