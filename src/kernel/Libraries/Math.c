#include "Libraries/Math.h"

int round(int x){
    if (x >= 0) {
        int t = (int)(x + 0.5f);   // truncates toward 0
        return t;
    } else {
        int t = (int)(x - 0.5f);   // truncates toward 0
        return t;
    }
}

uint64_t abs(uint64_t n){
    if (n < 0){
       return -n;
    }
    return n;
}

uint64_t last_unsecure_64 = 4;

uint64_t rand64(){
    last_unsecure_64 = (6364136223846793005ULL*last_unsecure_64+1);
    return last_unsecure_64;
}