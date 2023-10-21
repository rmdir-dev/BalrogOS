#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "balrog/terminal/term.h"

int k;

char b[1761];
float z[1761];
float A=0;
char tmp_screen_buffer[80 * 25];

void print_screen_buffer()
{
    printf(TERM_CLEAR);
    printf(tmp_screen_buffer);
}

int main(int argc, char** argv){
    float A = 0.5;
    float B=0;
    float i;
    float j;


    printf(TERM_CLEAR);
    for(; ; ) {
        memset(b,32,1760);
        memset(z,0,7040);
        for(j=0; 6.28>j; j+=(0.14 * 2)) {
            for(i=0; 6.28 >i; i+=(0.08 * 2)) {
                float sini=sin(i),
                        cosj=cos(j),
                        sinA=sin(A),
                        sinj=sin(j),
                        cosA=cos(A),
                        cosj2=cosj+2,
                        mess=1/(sini*cosj2*sinA+sinj*cosA+5),
                        cosi=cos(i),
                        cosB=cos(B),
                        sinB=sin(B),
                        t=sini*cosj2*cosA-sinj* sinA;
                int x=40+30*mess*(cosi*cosj2*cosB-t*sinB),
                        y= 12+15*mess*(cosi*cosj2*sinB +t*cosB),
                        o=(x+80*y)%1760,
                        N=8*((sinj*sinA-sini*cosj*cosA)*cosB-sini*cosj*sinA-sinj*cosA-cosi *cosj*sinB);
                if(22>y&&y>0&&x>0&&80>x&&mess>z[o]){
                    z[o]=mess;
                    b[o]=".,-~:;=!*#$@"[N>0?N%11:0];
                }
            }
        }

        for(k=0; 1761>k; k++)
        {
            tmp_screen_buffer[k] = (char) ((k % 80 ? b[k] : 10));
        }
        print_screen_buffer();

        A+=0.04;
        B+= 0.02;
    }

    return 0;
}