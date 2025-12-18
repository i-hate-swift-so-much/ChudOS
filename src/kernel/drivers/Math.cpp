#include "Math.h"
#include "stdint.h"

double sqrt(double n){
    // uses Newton-Raphson algorithm
    double x = n;
    double y = 0.5 * (x + n / x);
    while (y!= x){
        x = y;
        y = 0.5 * (x + n / x);
    }
    return x;
}

int round(int x){
    if (x >= 0) {
        int t = (int)(x + 0.5f);   // truncates toward 0
        return t;
    } else {
        int t = (int)(x - 0.5f);   // truncates toward 0
        return t;
    }
}

int abs(int n){
    if (n < 0){
       return -n;
    }
    return n;
}