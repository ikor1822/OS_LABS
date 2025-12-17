#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#define MIN_ORDER 4

typedef struct Header {
    int order;
    struct Header* next;
} Header;

typedef struct {
    void* base;
    size_t total_size;
    int min_order;
    int max_order;
    Header** free_lists;
} BuddyAllocator;

static inline size_t offset_of(BuddyAllocator* a, Header* h) {
    return (size_t)((char*)h - (char*)a->base);
}

static inline Header* ptr_from_offset(BuddyAllocator* a, size_t off) {
    return (Header*)((char*)a->base + off);
}

static int remove_from_list(Header** list, Header* h) {
    Header* prev = NULL;
    Header* cur = *list;
    while (cur) {
        if (cur == h) {
            if (prev) prev->next = cur->next;
            else *list = cur->next;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

int createBuddyAllocator(BuddyAllocator* a, size_t size) {
    if (!a || size < (1UL << MIN_ORDER))
        return 0;

    void* mem = mmap(NULL, size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);

    if (mem == MAP_FAILED)
        return 0;

    a->base = mem;
    a->total_size = size;
    a->min_order = MIN_ORDER;

    size_t t = size;
    int mo = 0;
    while (t >>= 1) mo++;
    a->max_order = mo;

    int lists = a->max_order - a->min_order + 1;

    a->free_lists = mmap(NULL,
                          lists * sizeof(Header*),
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);

    if (a->free_lists == MAP_FAILED) {
        munmap(mem, size);
        return 0;
    }

    for (int i = 0; i < lists; i++)
        a->free_lists[i] = NULL;

    Header* h = (Header*)a->base;
    h->order = a->max_order;
    h->next = NULL;
    a->free_lists[a->max_order - a->min_order] = h;

    return 1;
}

void* buddy_alloc(BuddyAllocator* a, size_t size) {
    if (!a || size == 0)
        return NULL;

    size_t need = size + sizeof(Header);
    size_t blk = 1;
    int order = 0;

    while (blk < need) {
        blk <<= 1;
        order++;
    }

    if (order < a->min_order)
        order = a->min_order;
    if (order > a->max_order)
        return NULL;

    int i = order;
    while (i <= a->max_order &&
           a->free_lists[i - a->min_order] == NULL)
        i++;

    if (i > a->max_order)
        return NULL;

    Header* cur = a->free_lists[i - a->min_order];
    a->free_lists[i - a->min_order] = cur->next;
    cur->next = NULL;

    while (i > order) {
        i--;
        size_t off = offset_of(a, cur);
        size_t half = (size_t)1 << i;

        Header* left = cur;
        Header* right = ptr_from_offset(a, off + half);

        left->order = i;
        right->order = i;

        right->next = a->free_lists[i - a->min_order];
        a->free_lists[i - a->min_order] = right;

        cur = left;
    }

    cur->order = order;
    return (char*)cur + sizeof(Header);
}

void buddy_free(BuddyAllocator* a, void* ptr) {
    if (!a || !ptr)
        return;

    Header* cur = (Header*)((char*)ptr - sizeof(Header));
    int order = cur->order;

    while (order < a->max_order) {
        size_t off = offset_of(a, cur);
        size_t buddy_off = off ^ ((size_t)1 << order);
        Header* buddy = ptr_from_offset(a, buddy_off);

        Header** list = &a->free_lists[order - a->min_order];
        if (!remove_from_list(list, buddy))
            break;

        if ((char*)buddy < (char*)cur)
            cur = buddy;

        order++;
        cur->order = order;
    }

    cur->next = a->free_lists[order - a->min_order];
    a->free_lists[order - a->min_order] = cur;
}

size_t buddy_free_memory(BuddyAllocator* a) {
    if (!a)
        return 0;

    size_t sum = 0;
    for (int o = a->min_order; o <= a->max_order; o++) {
        Header* h = a->free_lists[o - a->min_order];
        while (h) {
            sum += (size_t)1 << o;
            h = h->next;
        }
    }
    return sum;
}

void destroyBuddyAllocator(BuddyAllocator* a) {
    if (!a)
        return;

    int lists = a->max_order - a->min_order + 1;
    munmap(a->free_lists, lists * sizeof(Header*));
    munmap(a->base, a->total_size);
}
