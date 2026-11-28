#include <math.h>

float pi(int k) {
    if (k <= 0) return 0.0f;
    double acc = 0.0;
    double sign = 1.0;
    for (int step = 0; step < k; ++step) {
        acc += sign / (2.0 * step + 1.0);
        sign = -sign;
    }
    return (float)(4.0 * acc);
}

float e(int x) {
    if (x <= 0) return 1.0f;
    double base = 1.0 + 1.0 / (double)x;
    return (float)pow(base, (double)x);
}