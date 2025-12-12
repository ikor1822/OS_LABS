#include <stdio.h>
#include "mathlib.h"

int main() {
    int cmd, arg;

    while (1) {
        if (scanf("%d", &cmd) == EOF)
            break;

        if (cmd == -1) {
            printf("Exit\n");
            break;
        }

        if (cmd == 1) {
            scanf("%d", &arg);
            printf("Pi = %f\n", Pi(arg));
        }
        else if (cmd == 2) {
            scanf("%d", &arg);
            printf("E = %f\n", E(arg));
        }
        else {
            printf("нет такой команды\n");
        }
    }

    return 0;
}
