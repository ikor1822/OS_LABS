#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#define PAGE_SIZE 4096U
#define MKC_PAGE_FREE  0xFFFFu
#define MKC_PAGE_LARGE 0xFFFEu

static const size_t mkc_classes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};
#define MKC_NUM_CLASSES (sizeof(mkc_classes)/sizeof(mkc_classes[0]))

typedef struct MKC_Page {
    uint16_t size_class_index;
    uint16_t free_count;
    struct MKC_Page* next;
    uint32_t bitmap[8];
} MKC_Page;

typedef struct Allocator {
    size_t total_size;
    void* base_data;
    size_t pages_count;

    MKC_Page* free_pages;
    MKC_Page* class_pages[MKC_NUM_CLASSES];
    MKC_Page* large_pages;
} Allocator;

static inline MKC_Page* page_at(Allocator* a, size_t i) {
    return (MKC_Page*)((char*)a->base_data + i * PAGE_SIZE);
}

static inline void bitmap_clear(uint32_t* b) {
    memset(b, 0, sizeof(uint32_t) * 8);
}

static inline int bitmap_find(uint32_t* b, int limit) {
    for (int i = 0; i < limit; i++)
        if (!(b[i >> 5] & (1u << (i & 31))))
            return i;
    return -1;
}

static inline int slots_in_class(int ci) {
    int s = (PAGE_SIZE - sizeof(MKC_Page)) / mkc_classes[ci];
    return s > 256 ? 256 : s;
}

static inline int class_index(size_t size) {
    for (int i = 0; i < MKC_NUM_CLASSES; i++)
        if (size <= mkc_classes[i])
            return i;
    return -1;
}

static MKC_Page* take_free(Allocator* a) {
    MKC_Page* p = a->free_pages;
    if (!p) return NULL;
    a->free_pages = p->next;
    p->next = NULL;
    bitmap_clear(p->bitmap);
    return p;
}

static void put_free(Allocator* a, MKC_Page* p) {
    p->size_class_index = MKC_PAGE_FREE;
    p->free_count = 0;
    bitmap_clear(p->bitmap);
    p->next = a->free_pages;
    a->free_pages = p;
}

Allocator* createMKCAllocator(size_t size) {
    if (size < PAGE_SIZE * 2)
        return NULL;
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
        return NULL;
    Allocator* a = (Allocator*)mem;
    memset(a, 0, sizeof(Allocator));
    a->total_size = size;
    a->base_data = (char*)mem + PAGE_SIZE;
    a->pages_count = (size - PAGE_SIZE) / PAGE_SIZE;
    for (size_t i = 0; i < a->pages_count; i++) {
        MKC_Page* p = page_at(a, i);
        p->size_class_index = MKC_PAGE_FREE;
        bitmap_clear(p->bitmap);
        p->next = a->free_pages;
        a->free_pages = p;
    }
    return a;
}

void destroyMKCAllocator(Allocator* a) {
    if (!a) return;
    munmap(a, a->total_size);
}

void* alloc(Allocator* a, size_t size) {
    if (!a || size == 0)
        return NULL;

    int ci = class_index(size);

    if (ci >= 0) {
        MKC_Page* pg = a->class_pages[ci];
        while (pg && pg->free_count == 0)
            pg = pg->next;

        if (!pg) {
            pg = take_free(a);
            if (!pg) return NULL;
            pg->size_class_index = ci;
            pg->free_count = slots_in_class(ci);
            pg->next = a->class_pages[ci];
            a->class_pages[ci] = pg;
        }

        int slot = bitmap_find(pg->bitmap, slots_in_class(ci));
        if (slot < 0) return NULL;

        pg->bitmap[slot >> 5] |= 1u << (slot & 31);
        pg->free_count--;

        return (char*)pg + sizeof(MKC_Page)
             + slot * mkc_classes[ci];
    }

    size_t need = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t run = 0, start = 0;
    int found = 0;

    for (size_t i = 0; i < a->pages_count; i++) {
        MKC_Page* p = page_at(a, i);
        if (p->size_class_index == MKC_PAGE_FREE) {
            if (!run) start = i;
            if (++run == need) {
                found = 1;
                break;
            }
        } else {
            run = 0;
        }
    }

    if (!found) return NULL;

    MKC_Page* first = page_at(a, start);
    first->size_class_index = MKC_PAGE_LARGE;
    first->free_count = need;

    for (size_t i = 1; i < need; i++)
        page_at(a, start + i)->size_class_index = MKC_PAGE_LARGE;

    MKC_Page** pp = &a->free_pages;
    while (*pp) {
        MKC_Page* p = *pp;
        size_t idx = ((char*)p - (char*)a->base_data) / PAGE_SIZE;
        if (idx >= start && idx < start + need) {
            *pp = p->next;
            continue;
        }
        pp = &p->next;
    }

    first->next = a->large_pages;
    a->large_pages = first;

    return (char*)first + sizeof(MKC_Page);
}

void free_block(Allocator* a, void* ptr) {
    if (!a || !ptr)
        return;

    size_t off = (char*)ptr - (char*)a->base_data;
    size_t idx = off / PAGE_SIZE;
    if (idx >= a->pages_count)
        return;

    MKC_Page* pg = page_at(a, idx);

    if (pg->size_class_index == MKC_PAGE_LARGE) {
        size_t n = pg->free_count;
        for (size_t i = 0; i < n; i++)
            put_free(a, page_at(a, idx + i));
        return;
    }

    int ci = pg->size_class_index;
    size_t cls = mkc_classes[ci];
    size_t slot =
        ((char*)ptr - ((char*)pg + sizeof(MKC_Page))) / cls;

    if (!(pg->bitmap[slot >> 5] & (1u << (slot & 31))))
        return;

    pg->bitmap[slot >> 5] &= ~(1u << (slot & 31));
    pg->free_count++;

    if (pg->free_count == slots_in_class(ci)) {
        MKC_Page** pp = &a->class_pages[ci];
        while (*pp) {
            if (*pp == pg) {
                *pp = pg->next;
                break;
            }
            pp = &(*pp)->next;
        }
        put_free(a, pg);
    }
}

size_t get_free_memory(Allocator* a) {
    if (!a) return 0;

    size_t sum = 0;
    for (MKC_Page* p = a->free_pages; p; p = p->next)
        sum += PAGE_SIZE;

    for (size_t i = 0; i < MKC_NUM_CLASSES; i++) {
        MKC_Page* pg = a->class_pages[i];
        while (pg) {
            sum += pg->free_count * mkc_classes[i];
            pg = pg->next;
        }
    }
    return sum;
}
