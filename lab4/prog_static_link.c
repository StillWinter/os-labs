#include <stdio.h>

float pi(int k);
float e(int x);

int main(void) {
    int cmd = 0;
    int arg = 0;

    while (scanf("%d", &cmd) == 1) {
        if (cmd == 1) {
            if (scanf("%d", &arg) != 1) break;
            printf("pi(%d) = %.6f\n", arg, pi(arg));
        } else if (cmd == 2) {
            if (scanf("%d", &arg) != 1) break;
            printf("e(%d) = %.6f\n", arg, e(arg));
        } else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}