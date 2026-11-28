float pi(int k) {
    if (k <= 0) return 0.0f;
    double prod = 1.0;
    for (int idx = 1; idx <= k; ++idx) {
        double num = 4.0 * idx * idx;
        prod *= num / (num - 1.0);
    }
    return (float)(2.0 * prod);
}

float e(int x) {
    if (x < 0) return 0.0f;
    double total = 1.0;
    double term = 1.0;
    for (int n = 1; n <= x; ++n) {
        term /= (double)n;
        total += term;
    }
    return (float)total;
}