#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

typedef unsigned long long ull;

pthread_mutex_t mutex;
ull global_success = 0;

void* worker(void* arg) {
    ull rounds = *(ull*)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();
    ull local_success = 0;

    for (ull i = 0; i < rounds; i++) {
        int a = rand_r(&seed) % 52;
        int b = rand_r(&seed) % 51;
        if (b >= a) b++;

        if ((a % 13) == (b % 13))
            local_success++;
    }

    pthread_mutex_lock(&mutex);
    global_success += local_success;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    int threads = -1;
    ull rounds = 0;
    int opt;
    while ((opt = getopt(argc, argv, "t:n:")) != -1) {
        switch (opt) {
            case 't': threads = atoi(optarg); break;
            case 'n': rounds = strtoull(optarg, NULL, 10); break;
        }
    }

    if (threads <= 0 || rounds == 0) {
        fprintf(stderr, "Использование: %s -t <потоки> -n <раунды>\n", argv[0]);
        return 1;
    }

    pthread_t* ths = malloc(sizeof(pthread_t) * threads);
    ull* args = malloc(sizeof(ull) * threads);
    ull base = rounds / threads;
    ull rem = rounds % threads;

    pthread_mutex_init(&mutex, NULL);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < threads; i++) {
        args[i] = base + (i < (int)rem ? 1 : 0);
        pthread_create(&ths[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < threads; i++)
        pthread_join(ths[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    double probability = (double)global_success / (double)rounds;

    printf("Потоков: %d\n", threads);
    printf("Раундов: %llu\n", rounds);
    printf("Вероятность совпадения: %.6f\n", probability);
    printf("Время выполнения: %.6f сек\n", elapsed);

    pthread_mutex_destroy(&mutex);
    free(ths);
    free(args);
    return 0;
}