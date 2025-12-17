#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "buddy.c"
#include "mkc.c"

#define MEMORY_SIZE (4 * 1024 * 1024)
#define NUM_OPERATIONS 100000
#define MAX_BLOCK_SIZE 128

double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void test_buddy() {
    printf("Алгоритм двойников\n");

    BuddyAllocator alloc;
    if (!createBuddyAllocator(&alloc, MEMORY_SIZE)) {
        printf("init\n");
        return;
    }

    void** ptrs = malloc(NUM_OPERATIONS * sizeof(void*));

    double t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = rand() % MAX_BLOCK_SIZE + 1;
        ptrs[i] = buddy_alloc(&alloc, s);
    }
    double t2 = now_sec();
    printf("alloc: %f s\n", t2 - t1);

    t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++)
        buddy_free(&alloc, ptrs[i]);
    t2 = now_sec();
    printf("free:  %f s\n", t2 - t1);

    destroyBuddyAllocator(&alloc);
    if (!createBuddyAllocator(&alloc, MEMORY_SIZE)) {
        printf("reinit\n");
        free(ptrs);
        return;
    }

    size_t total_req = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = rand() % MAX_BLOCK_SIZE + 1;
        ptrs[i] = buddy_alloc(&alloc, s);
        if (ptrs[i])
            total_req += s;
    }

    size_t free_mem = buddy_free_memory(&alloc);
    double used = (double)(MEMORY_SIZE - free_mem);
    double util = used > 0 ? (double)total_req / used : 0.0;

    printf("utilization: %.2f %%\n\n", util * 100.0);

    for (int i = 0; i < NUM_OPERATIONS; i++)
        buddy_free(&alloc, ptrs[i]);

    destroyBuddyAllocator(&alloc);
    free(ptrs);
}

void test_mkc() {
    printf("Мак-Кьзи\n");

    Allocator* a = createMKCAllocator(MEMORY_SIZE);
    if (!a) {
        printf("init\n");
        return;
    }

    void** ptrs = malloc(NUM_OPERATIONS * sizeof(void*));

    double t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = rand() % MAX_BLOCK_SIZE + 1;
        ptrs[i] = alloc(a, s);
    }
    double t2 = now_sec();
    printf("alloc: %f s\n", t2 - t1);

    t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++)
        free_block(a, ptrs[i]);
    t2 = now_sec();
    printf("free:  %f s\n", t2 - t1);

    destroyMKCAllocator(a);

    a = createMKCAllocator(MEMORY_SIZE);
    if (!a) {
        printf("reinit\n");
        free(ptrs);
        return;
    }

    size_t total_req = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = rand() % MAX_BLOCK_SIZE + 1;
        ptrs[i] = alloc(a, s);
        if (ptrs[i])
            total_req += s;
    }

    size_t free_mem = get_free_memory(a);
    double used = (double)(MEMORY_SIZE - free_mem);
    double util = used > 0 ? (double)total_req / used : 0.0;

    printf("utilization: %.2f %%\n\n", util * 100.0);

    for (int i = 0; i < NUM_OPERATIONS; i++)
        free_block(a, ptrs[i]);

    destroyMKCAllocator(a);
    free(ptrs);
}

int main() {
    srand(1234567);

    test_buddy();
    test_mkc();

    return 0;
}
