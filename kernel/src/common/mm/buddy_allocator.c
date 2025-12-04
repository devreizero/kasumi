#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <mm/buddy_allocator.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/zone.h>
#include <macros.h>

static void pushFreeBlock(struct FreeList *list, struct FreeBlock *block);
static struct FreeBlock * popFreeBlock(struct FreeList *list);

static inline struct FreeBlock *buddyOf(const struct FreeBlock *block, const size_t order) {
    return (struct FreeBlock *) ((uintptr_t) block ^ ((1ULL << order) * PAGE_SIZE));
}

static inline size_t pfnOf(const void *addr, const struct Zone *zone) {
    assert((uintptr_t) addr >= zone->base);
    return ((uintptr_t) addr - zone->base) >> PAGE_SHIFT;
}

static inline struct FreeBlock *blockOf(const size_t index, const struct Zone *zone) {
    return (struct FreeBlock *) (zone->base + index * PAGE_SIZE);
}

void *buddyAllocAligned(struct Zone *zone, size_t order, size_t align) {
    struct Buddy *buddy = zone->buddy;

    if (order >= BUDDY_MAX_ORDER || buddy->freePages <= 0) {
        return NULL;
    }

    size_t currentOrder = order;

    // Search free block by going to larger-size pool
    while (currentOrder < BUDDY_MAX_ORDER && buddy->freeLists[currentOrder].head == NULL) {
        ++currentOrder;
    }

    if (currentOrder == BUDDY_MAX_ORDER) return NULL;

    struct FreeBlock *block = popFreeBlock(buddy->freeLists + currentOrder);
    struct FreeBlock *reclaimedBlock;

    // Split block until wanted order
    while (currentOrder > order) {
        --currentOrder;
        reclaimedBlock = (struct FreeBlock *) buddyOf(block, currentOrder);
        pushFreeBlock(buddy->freeLists + currentOrder, (struct FreeBlock *) hhdmAdd(reclaimedBlock));
    }

    // Optional alignment
    uintptr_t addr = (uintptr_t) block;
    uintptr_t alignedAddr = __alignup(addr, align ? align : PAGE_SIZE);

    // If alignedAddr differ from addr: push padding to free list
    size_t paddingPages = (alignedAddr - addr) >> PAGE_SHIFT;
    if (paddingPages > 0) {
        struct FreeBlock *padBlock = block;
        // calculate biggest order that the padding fit into
        size_t padOrder = 0, pages = 1;
        while (pages * 2 <= paddingPages) { pages <<= 1; ++padOrder; }
        pushFreeBlock(buddy->freeLists + padOrder, (struct FreeBlock *) hhdmAdd(padBlock));
        buddy->freePages -= ((1 << order) - pages); // update freePages
    }

    size_t index = pfnOf(block, zone);
    for (size_t i = 0; i < (1 << order); i++) {
        buddy->pageOrders[index + i] = order;
    }

    buddy->freePages -= (1 << order);

    return block;
}

void *buddyAlloc(struct Zone *zone, size_t order) {
    return buddyAllocAligned(zone, order, 0);
}


void buddyFree(struct Zone *zone, void *pptr) {
    struct Buddy *buddy = zone->buddy;
    struct FreeBlock **prev;
    struct FreeBlock *block;

    size_t index = pfnOf(pptr, zone);
    size_t order = buddy->pageOrders[index];
    size_t buddyIndex;
    size_t npages;

    while (order < BUDDY_MAX_ORDER) {
        buddyIndex = index ^ (1 << order);

        if (buddy->pageOrders[buddyIndex] != order) {
            break;
        }

        prev  = &buddy->freeLists[order].head;
        block = *prev;

        while (block && (size_t) pfnOf(block, zone) != buddyIndex) {
            prev  = &block->next;
            block = block->next;
        }

        if (!block) {
            break;
        }

        --buddy->freeLists[order].count;
        if (buddyIndex < index) {
            index = buddyIndex;
        }

        ++order;
    }

    block = (struct FreeBlock *) hhdmAdd(blockOf(index, zone));
    pushFreeBlock(buddy->freeLists + order, block);

    npages = (1 << order);
    for (size_t i = 0; i < npages; i++) {
        buddy->pageOrders[index + i] = order;
    }

    buddy->freePages += npages;
}

static void pushFreeBlock(struct FreeList *list, struct FreeBlock *block) {
    block->next =  list->head;
    list->head  = block;
    ++list->count;
}

static struct FreeBlock * popFreeBlock(struct FreeList *list) {
    if (!list->head) {
        return NULL;
    }

    struct FreeBlock *block = (struct FreeBlock *) hhdmRemove(list->head);
    list->head = list->head->next;
    --list->count;

    return block;
}