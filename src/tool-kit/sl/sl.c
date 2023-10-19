//
// Created by rmdir on 10/18/23.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <balrog/terminal/term.h>

/*
    o o o o o o o . . .   ______________________________ _____=======_||____
   o      _____           ||                            | |                 |
 .][__n_n_|DD[  ====_____  |                            | |                 |
>(________|__|_[_________]_|____________________________|_|_________________|
_/oo OOOOO oo`  ooo   ooo  'o!o!o                  o!o!o` 'o!o         o!o`
 */

char train[5][78] = {
"    o o o o o o o . . .   ______________________________ _____=======_||____",
"   o      _____           ||                            | |                 |",
" .][__n_n_|DD[  ====_____  |                            | |                 |",
">(________|__|_[_________]_|____________________________|_|_________________|",
"_/oo OOOOO oo`  ooo   ooo  'o!o!o                  o!o!o` 'o!o         o!o`   "
};

void clear() {
    printf(TERM_CLEAR);
}

void main(int argc, char** argv) {
    clear();
    for(int a = 2 * 80; a > 0; --a) {
        clear();
        for (int j = 0; j < 25; ++j) {
            for (int i = 0; i < 80; ++i) {
                if(j < 12 || j > 16 || (i < a % 81 && a > 80) || (i >= a && a <= 80)) {
                    printf(" ");
                } else {
                    if(a > 80) {
                        printf("%c", train[j - 12][(i - (a % 81)) % 78]);
                    } else {
                        int c = (i - (a % 81));
                        if(c < 0) {
                            c += 80;
                        }
                        if(c >= 78) {
                            printf(" ");
                        } else {
                            printf("%c", train[j - 12][c % 78]);
                        }
                    }
                }
            }
//            printf("\n");
        }
        for(int w = 0; w < 20000000; w++) {

        }
    }

    clear();
    exit(0);
}