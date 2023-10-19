//
// Created by rmdir on 10/19/23.
//

#ifndef BALROGOS_USER_MANAGER_H
#define BALROGOS_USER_MANAGER_H

typedef struct _user_data {
    char* name;
    uint32_t uid;
    uint32_t gid;
    char* home;
    char* shell;
} __attribute__((packed)) user_data;

user_data* usm_get_user_data(uint32_t uid);

void init_user_manager();

#endif //BALROGOS_USER_MANAGER_H
