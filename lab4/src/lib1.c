#include <math.h>
#include "mathlib.h"

float Pi(int K) {
    float pi = 0.0f;
    for(int n = 0; n < K; n++)
        pi += powf(-1, n) / (2*n + 1);
    return pi * 4;
}
float E(int x) {
    return powf(1.0f + 1.0f/x, x);
}
