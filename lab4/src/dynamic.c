#include <stdio.h>
#include <dlfcn.h>
#include "mathlib.h"

int main() {
    char *libs[] = {"./lib1.so", "./lib2.so"};
    int cur = 0;

    void *handle = dlopen(libs[cur], RTLD_LAZY);
    if (!handle) {
        printf("dlerror: %s\n", dlerror());
        return 1;
    }

    float (*PiFunc)(int) = dlsym(handle, "Pi");
    float (*EFunc)(int)  = dlsym(handle, "E");

    int cmd, arg;

    while (1) {
        if (scanf("%d", &cmd) == EOF)
            break;

        if (cmd == -1) {
            printf("Exit\n");
            break;
        }

        if (cmd == 0) {
            dlclose(handle);
            cur = 1 - cur;

            handle = dlopen(libs[cur], RTLD_LAZY);
            if (!handle) {
                printf("dlopen error: %s\n", dlerror());
                return 1;
            }

            PiFunc = dlsym(handle, "Pi");
            EFunc  = dlsym(handle, "E");

            printf("Library swapped\n");
        }

        else if (cmd == 1) {
            scanf("%d", &arg);
            printf("Pi = %f\n", PiFunc(arg));
        }

        else if (cmd == 2) {
            scanf("%d", &arg);
            printf("E = %f\n", EFunc(arg));
        }

        else {
            printf("нет такой команды\n");
        }
    }

    dlclose(handle);
    return 0;
}
