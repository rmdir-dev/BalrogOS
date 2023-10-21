#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/Memory/kstack.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/GDT/gdt.h"
#include "BalrogOS/CPU/RFLAGS/rflag.h"
#include "BalrogOS/Tasking/Elf/elf.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/User/user_manager.h"
#include "balrog/memory/proc_mem.h"
#include "BalrogOS/Debug/debug_output.h"
#include "klib/DataStructure/rbt.h"
#include <string.h>
#include <stdlib.h>

rbt_tree user_tree;

user_data* usm_get_user_data(uint32_t uid) {
    rbt_node* node = rbt_search(&user_tree, uid);

    if(node == NULL) {
        kernel_debug_output(KDB_LVL_ERROR, "user does not exist !");
        while(1){}
    }

    return node->value;
}



void init_user_manager() {
    rbt_init(&user_tree);

    fs_fd fd;
    fs_file file;
    fs_get_file("/etc/shadow", &file, &fd);

    char* file_data = vmalloc(file.size);
    void* original_address = file_data;
    memcpy(file_data, file.data, file.size);
    file_data[file.size] = 0;
    fs_close(&fd);

    uint32_t len = strlen(file_data);
    file_data[len] = 0;
    uint32_t current_pos = 0;

    char* line = strtok(file_data, '\n');
    while(line != NULL) {
        uint32_t len = strlen(line);
        current_pos += len + 1;
        line[len] = 0;
        if(line[0] == '#') {
            line = strtok(NULL, '\n');
            continue;
        }

        char* user = strtok(line, ':');
        char* pass = strtok(NULL, ':');
        char* uid = strtok(NULL, ':');
        char* gid = strtok(NULL, ':');
        char* home = strtok(NULL, ':');
        char* shell = strtok(NULL, ':');

        user_data* c_usr = vmalloc(sizeof(user_data));
        c_usr->uid = atoi(uid);
        c_usr->gid = atoi(gid);
        c_usr->home = vmalloc(strlen(home) + 1);
        memcpy(c_usr->home, home, strlen(home) + 1);
        c_usr->shell = vmalloc(strlen(shell) + 1);
        memcpy(c_usr->shell, shell, strlen(shell) + 1);
        c_usr->name = vmalloc(strlen(user) + 1);
        memcpy(c_usr->name, user, strlen(user) + 1);
        rbt_node* node = rbt_insert(&user_tree, c_usr->uid);
        node->value = c_usr;

        if(current_pos >= file.size) {
            break;
        }

        line = strtok(&file_data[current_pos], '\n');
    }

    vmfree(original_address);
}