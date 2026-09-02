//
// Created by rmdir on 10/22/23.
//
#include <sys/debug.h>

int main(int argc, char** argv) {
    if(argc != 2)
    {
        return -1;
    }

    __sys_debug(atoi(argv[1]));

    return 0;
}