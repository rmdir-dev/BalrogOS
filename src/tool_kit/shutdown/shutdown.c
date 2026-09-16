#include <unistd.h>
#include <string.h>
#include "balrog/system/reboot.h"

int main(int argc, char** argv)
{
    int is_reboot = (argc > 1 && strcmp(argv[1], "-r") == 0);
    return reboot(is_reboot ? BALROG_REBOOT_RESTART : BALROG_REBOOT_POWER_OFF);
}