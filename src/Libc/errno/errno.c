//
// Created by rmdir on 10/20/23.
//

#define ERRNO_ADDRESS ((int*) (0x00007ffd0e212000 + (0x1000 - 16)))

int* __get_errno()
{
    return ERRNO_ADDRESS;
}