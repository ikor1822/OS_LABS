#include "mathlib.h"

float Pi(int K) {
    float pi = 1.0f;
    for(int n = 1; n <= K; n++)
        pi *= (4.0f*n*n) / (4.0f*n*n - 1);
    return pi * 2;
}
float E(int x) {
    float e = 1.0f;
    float fact = 1.0f;
    for(int n = 1; n <= x; n++) {
        fact *= n;
        e += 1.0f / fact;
    }
    return e;
}
