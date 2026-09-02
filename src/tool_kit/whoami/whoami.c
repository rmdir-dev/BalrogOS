//
// Created by rmdir on 10/20/23.
//

#include "toolkit/tool_read_keyboard.h"

int main(int argc, char** argv)
{
    user_info_t info = {};
    get_user_info(&info);

    printf("%s\n", info.username);
    return 0;
}
