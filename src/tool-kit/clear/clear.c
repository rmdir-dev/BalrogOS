#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <balrog/terminal/term.h>

int main(int argc, char** argv)
{
    printf(TERM_CLEAR);
    return 0;
}