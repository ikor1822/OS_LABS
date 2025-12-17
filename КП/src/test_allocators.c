#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "buddy.c"
#include "mkc.c"
#define MEMORY_SIZE (10 * 1024 *1024)
#define NUM_OPERATIONS 100000
#define MAX_BLOCK_SIZE 128

double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void test_buddy() {
    printf("Алгоритм двойников\n");
    static unsigned char memory[MEMORY_SIZE];
    static BuddyAllocator alloc;
    static Header* freelists[64];
    buddy_init(&alloc, memory, MEMORY_SIZE, freelists);
    void** pointers = malloc(NUM_OPERATIONS * sizeof(void*));
    double t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = (rand() % MAX_BLOCK_SIZE) + 1;
        pointers[i] = buddy_alloc(&alloc, s);
    }
    double t2 = now_sec();
    printf("alloc: %f с\n", t2 - t1);
    t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        buddy_free(&alloc, pointers[i]);
    }
    t2 = now_sec();
    printf("free: %f с\n", t2 - t1);
    buddy_init(&alloc, memory, MEMORY_SIZE, freelists);
    size_t total_req = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = (rand() % MAX_BLOCK_SIZE) + 1;
        void* p = buddy_alloc(&alloc, s);
        pointers[i] = p;
        if (p) total_req += s;
    }
    size_t free_mem = buddy_free_memory(&alloc);
    double used = (double)(MEMORY_SIZE - free_mem);
    double util = total_req / used;
    printf("utilization: %.2f %%\n\n", util * 100.0);
    for (int i = 0; i < NUM_OPERATIONS; i++)
        buddy_free(&alloc, pointers[i]);

    free(pointers);
}

void test_mkc() {
    printf("Мак-Кьзи\n");
    static unsigned char memory[MEMORY_SIZE];
    Allocator* mkc = createMemoryAllocator(memory, MEMORY_SIZE);
    void** pointers = malloc(NUM_OPERATIONS * sizeof(void*));
    double t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = (rand() % MAX_BLOCK_SIZE) + 1;
        pointers[i] = alloc(mkc, s);
    }
    double t2 = now_sec();
    printf("alloc: %f с\n", t2 - t1);
    t1 = now_sec();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        free_block(mkc, pointers[i]);
    }
    t2 = now_sec();
    printf("free: %f с\n", t2 - t1);
    mkc = createMemoryAllocator(memory, MEMORY_SIZE);
    size_t total_req = 0;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        size_t s = (rand() % MAX_BLOCK_SIZE) + 1;
        void* p = alloc(mkc, s);
        pointers[i] = p;
        if (p) total_req += s;
    }
    size_t free_mem = get_free_memory(mkc);
    double used = (double)(MEMORY_SIZE - free_mem);
    double util = total_req / used;
    printf("utilization: %.2f %%\n\n", util * 100.0);
    for (int i = 0; i < NUM_OPERATIONS; i++)
        free_block(mkc, pointers[i]);

    free(pointers);
}

int main() {
    srand(1234567);

    test_buddy();
    test_mkc();

    return 0;
}
