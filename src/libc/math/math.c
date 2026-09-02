//
// Created by rmdir on 10/21/23.
//

#include <stdlib.h>
#include <math.h>

double fmod(double x, double y){
    return x - (int)(x/y)*y;
}

unsigned long fact(unsigned long n){
    if(n == 0)
        return 1;

    for (long i = n-1; i > 0; i--) {
        n*=i;
    }

    return n;
}

double pow(double x, double n){
    if(n==0)
        return 1;
    else
        return x*pow(x,n-1);
}

int abs(int x){
    if(x<0)
        return -x;
    else
        return x;
}

double sin(double x){
    double sum=0;
    x = fmod(x, 2*M_PI);

    for(long i=0; i<10; i++){
        sum+=(pow(-1,i)/(double) fact(2*i+1))*pow(x,2*i+1);
    }
    return sum;
}

double cos(double x){
    double sum=0;
    x = fmod(x, 2*M_PI);

    for(int i=0; i<10; i++){

        sum+=(pow(-1,i)/fact(2*i))*pow(x,2*i);
    }
    return sum;
}