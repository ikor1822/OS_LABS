#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

static inline uintptr_t offset_of(BuddyAllocator* a, Header* h) {
    return (uintptr_t)h - (uintptr_t)a->base;
}

static inline Header* ptr_from_offset(BuddyAllocator* a, uintptr_t offset) {
    return (Header*)((uintptr_t)a->base + offset);
}

void buddy_init(BuddyAllocator* a, void* memory, size_t size, Header** free_lists_buffer) {
    a->base = memory;
    a->total_size = size;
    a->min_order = MIN_ORDER;
    size_t t = size;
    int mo = 0;
    while (t >>= 1) mo++;
    a->max_order = mo;
    a->free_lists = free_lists_buffer;
    for (int i = 0; i <= a->max_order - a->min_order; i++)
        a->free_lists[i] = NULL;
    Header* h = (Header*)a->base;
    h->order = a->max_order;
    h->next = NULL;
    a->free_lists[a->max_order - a->min_order] = h;
}

static int remove_from_list(Header** list, Header* target) {
    Header* prev = NULL;
    Header* cur = *list;
    while (cur) {
        if (cur == target) {
            if (prev) prev->next = cur->next;
            else *list = cur->next;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

void* buddy_alloc(BuddyAllocator* a, size_t size) {
    if (size == 0) return NULL;
    size_t need = size + sizeof(Header);
    size_t blk = 1;
    int order = 0;
    while (blk < need) {
        blk <<= 1;
        order++;
    }
    if (order < a->min_order) order = a->min_order;
    if (order > a->max_order) return NULL;
    int i = order;
    while (i <= a->max_order && a->free_lists[i - a->min_order] == NULL)
        i++;
    if (i > a->max_order) return NULL;
    Header* cur = a->free_lists[i - a->min_order];
    a->free_lists[i - a->min_order] = cur->next;
    cur->next = NULL;
    while (i > order) {
        i--;
        uintptr_t off = offset_of(a, cur);
        size_t half = (size_t)1 << i;
        Header* left  = cur;
        Header* right = ptr_from_offset(a, off + half);
        left->order = i;
        right->order = i;
        right->next = a->free_lists[i - a->min_order];
        a->free_lists[i - a->min_order] = right;
        cur = left;
    }
    cur->order = order;
    return (void*)((uint8_t*)cur + sizeof(Header));
}

void buddy_free(BuddyAllocator* a, void* ptr) {
    if (!ptr) return;
    Header* h = (Header*)((uint8_t*)ptr - sizeof(Header));
    int order = h->order;
    Header* cur = h;
    while (order < a->max_order) {
        uintptr_t off = offset_of(a, cur);
        uintptr_t buddy_off = off ^ ((uintptr_t)1 << order);
        Header* buddy = ptr_from_offset(a, buddy_off);
        Header** list = &a->free_lists[order - a->min_order];
        if (!remove_from_list(list, buddy))
            break;
        if ((uintptr_t)buddy < (uintptr_t)cur)
            cur = buddy;
        order++;
        cur->order = order;
    }
    cur->next = a->free_lists[order - a->min_order];
    a->free_lists[order - a->min_order] = cur;
}

size_t buddy_free_memory(BuddyAllocator* a) {
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
