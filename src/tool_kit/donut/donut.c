#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "balrog/terminal/term.h"

/*
Not mine. The maths is Andy Sloane's obfuscated one liner, de-obfuscated : same
expressions, same magic numbers, same charset. What changed coming in is the
names, two bound guards on o and N, coarser steps, and our clear code. Only the
angle tables and the single write below are ours.

Credit : 
    Andy Sloane, donut.c : https://www.a1k0n.net/2011/07/20/donut-math.html
*/

#define DONUT_WIDTH     80
#define DONUT_HEIGHT    22
#define DONUT_CELLS     1760            // width * 22 rows
#define DONUT_FRAME     1761            // the cells plus the trailing newline

/*  the angle steps of the two loops. they never change, which is the whole
    reason the tables below work.  */
#define DONUT_J_STEP    (0.14 * 2)
#define DONUT_I_STEP    (0.08 * 2)
#define DONUT_J_COUNT   23
#define DONUT_I_COUNT   40

static char b[DONUT_FRAME];
static float z[DONUT_FRAME];

static char frame[sizeof(TERM_CLEAR) + DONUT_FRAME];

static float sin_i[DONUT_I_COUNT];
static float cos_i[DONUT_I_COUNT];
static float sin_j[DONUT_J_COUNT];
static float cos_j[DONUT_J_COUNT];

static void __donut_build_tables()
{
    float a = 0;

    for(int n = 0; n < DONUT_I_COUNT; n++)
    {
        sin_i[n] = sin(a);
        cos_i[n] = cos(a);
        a += DONUT_I_STEP;
    }

    a = 0;

    for(int n = 0; n < DONUT_J_COUNT; n++)
    {
        sin_j[n] = sin(a);
        cos_j[n] = cos(a);
        a += DONUT_J_STEP;
    }
}

int main(int argc, char** argv)
{
    float A = 0.5;
    float B = 0;

    __donut_build_tables();

    /*  the clear code sits at the head of the buffer once and never moves  */
    memcpy(frame, TERM_CLEAR, sizeof(TERM_CLEAR) - 1);
    char* out = frame + sizeof(TERM_CLEAR) - 1;

    printf(TERM_CLEAR);

    for(; ; )
    {
        memset(b, 32, DONUT_CELLS);
        memset(z, 0, sizeof(z));

        /*  the only four that still move, once a frame instead of once per
            point.  */
        float sinA = sin(A);
        float cosA = cos(A);
        float sinB = sin(B);
        float cosB = cos(B);

        for(int nj = 0; nj < DONUT_J_COUNT; nj++)
        {
            float sinj = sin_j[nj];
            float cosj = cos_j[nj];
            float cosj2 = cosj + 2;

            for(int ni = 0; ni < DONUT_I_COUNT; ni++)
            {
                float sini = sin_i[ni];
                float cosi = cos_i[ni];
                float mess = 1 / (sini * cosj2 * sinA + sinj * cosA + 5);
                float t = sini * cosj2 * cosA - sinj * sinA;

                int x = 40 + 30 * mess * (cosi * cosj2 * cosB - t * sinB),
                    y = 12 + 15 * mess * (cosi * cosj2 * sinB + t * cosB),
                    o = (x + DONUT_WIDTH * y) % DONUT_CELLS,
                    N = 8 * ((sinj * sinA - sini * cosj * cosA) * cosB - sini * cosj * sinA - sinj * cosA - cosi * cosj * sinB);

                if(DONUT_HEIGHT > y && y > 0 && x > 0 && DONUT_WIDTH > x && mess > z[o])
                {
                    z[o] = mess;
                    b[o] = ".,-~:;=!*#$@"[N > 0 ? N % 11 : 0];
                }
            }
        }

        for(int k = 0; k < DONUT_FRAME; k++)
        {
            out[k] = k % DONUT_WIDTH ? b[k] : '\n';
        }

        write(0, frame, (sizeof(TERM_CLEAR) - 1) + DONUT_FRAME);

        A += 0.04;
        B += 0.02;
    }

    return 0;
}
