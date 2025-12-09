#include <printf.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <mm/buddy_allocator.h>
#include <mm/pmm.h>
#include <mm/hhdm.h>
#include <mm/zone.h>
#include <macros.h>

// mm/buddy_allocator.c
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <mm/buddy_allocator.h>
#include <macros.h>
#include <printf.h>

#ifndef BUDDY_MAX_ORDER
#error "BUDDY_MAX_ORDER must be defined"
#endif

// Helpers to manipulate free lists and pageOrders.
// Convention: pageOrders[i] = (order & 0x7F) | (allocated ? 0x80 : 0x00)

static inline uintptr_t buddyBlockPhys(struct Buddy *b, size_t pageIndex) {
    return b->base + (pageIndex * PAGE_SIZE);
}

static inline size_t physToPageIndex(struct Buddy *b, uintptr_t phys) {
    return (phys - b->base) / PAGE_SIZE;
}

/* add a free block at given order; phys must be an aligned block base (phys within buddy range) */
static void addFreeBlock(struct Buddy *b, size_t order, uintptr_t phys) {
    struct FreeBlock *node = (struct FreeBlock *)hhdmAdd((void *)phys);
    node->next = b->freeLists[order].head;
    b->freeLists[order].head = node;
    b->freeLists[order].count++;
}

/* remove and return head of free list order (0 if none) */
static uintptr_t popFreeBlockHead(struct Buddy *b, size_t order) {
    struct FreeList *fl = &b->freeLists[order];
    if (!fl->head) return 0;
    struct FreeBlock *node = fl->head;
    fl->head = node->next;
    fl->count--;
    /* node exists at virtual = hhdmAdd(phys) but we need phys: convert back
       -> node pointer virtual -> subtract hhdm offset using hhdmRemove? 
       In your codebase you have hhdmRemoveAddr(). If not, we can compute phys by
       reading address of node and mapping to physical: but simplest: caller stores
       blocks only at physical base addresses equal to zone->base + ... so
       convert back with hhdmRemoveAddr((uintptr_t)node)
    */
    return hhdmRemoveAddr((uintptr_t)node);
}

/* remove arbitrary block phys from free list order; return true if removed */
static bool removeFreeBlock(struct Buddy *b, size_t order, uintptr_t phys) {
    struct FreeList *fl = &b->freeLists[order];
    struct FreeBlock *prev = NULL;
    struct FreeBlock *cur = fl->head;
    uintptr_t cur_phys;

    while (cur) {
        cur_phys = hhdmRemoveAddr((uintptr_t)cur);
        if (cur_phys == phys) {
            if (prev) prev->next = cur->next;
            else fl->head = cur->next;
            fl->count--;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

/* Set pageOrders for a block starting at pageIndex, for pages = (1<<order) */
static void setPageOrders(struct Buddy *b, size_t startPageIndex, size_t order) {
    size_t pages = (1ULL << order);
    for (size_t i = 0; i < pages; ++i) {
        assert(startPageIndex + i < b->totalPages);
        b->pageOrders[startPageIndex + i] = (uint8_t)order;
    }
}

void *buddyAllocAligned(struct Zone *zone, size_t order, size_t align) {
    if (!zone || !zone->buddy) return NULL;
    struct Buddy *b = zone->buddy;

    /* alignment in pages */
    size_t alignPages = 1;
    if (align > PAGE_SIZE) {
        if ((align & (align - 1)) != 0) {
            // not power-of-two alignment -> round up to next power-of-two
            size_t v = 1;
            while (v < align) v <<= 1;
            align = v;
        }
        alignPages = align / PAGE_SIZE;
    }

    /* compute minimal order that satisfies alignment: */
    size_t minOrderAlign = 0;
    while ((1ULL << minOrderAlign) < alignPages) ++minOrderAlign;

    size_t requiredOrder = order;
    if (minOrderAlign > requiredOrder) requiredOrder = minOrderAlign;

    if (requiredOrder >= BUDDY_MAX_ORDER) return NULL; // can't allocate

    /* find first non-empty list at level k >= requiredOrder */
    size_t k = requiredOrder;
    while (k < BUDDY_MAX_ORDER && b->freeLists[k].head == NULL) ++k;
    if (k >= BUDDY_MAX_ORDER) return NULL;

    /* Pop one block from level k */
    uintptr_t block_phys = popFreeBlockHead(b, k);
    if (!block_phys) return NULL;

    /* split down to requiredOrder */
    while (k > requiredOrder) {
        --k;
        /* sizes: pages_in_k = 1<<k */
        uintptr_t right_phys = block_phys + ((1ULL << k) * PAGE_SIZE);
        /* push right buddy into free list k */
        addFreeBlock(b, k, right_phys);

        /* update pageOrders for both halves */
        size_t leftPageIndex = physToPageIndex(b, block_phys);
        setPageOrders(b, leftPageIndex, k);
        size_t rightPageIndex = physToPageIndex(b, right_phys);
        setPageOrders(b, rightPageIndex, k);
        /* continue with left (block_phys stays same) */
    }

    /* final block_phys at order = requiredOrder
       Ensure that block_phys meets alignment (phys % align == 0).
       Given we enforced requiredOrder >= minOrderAlign, block size is multiple of align,
       and because splits aligned, block_phys should already be aligned. But double-check: */
    if (align > PAGE_SIZE) {
        if ((block_phys & (align - 1)) != 0) {
            // This should not happen normally because we increased requiredOrder,
            // but if it does, free this block and fail gracefully.
            addFreeBlock(b, requiredOrder, block_phys);
            setPageOrders(b, physToPageIndex(b, block_phys), requiredOrder);
            return NULL;
        }
    }

    /* mark pages as allocated: set pageOrders for the allocated pages to requiredOrder
       (this is important so free can find order) */
    size_t allocStartIndex = physToPageIndex(b, block_phys);
    setPageOrders(b, allocStartIndex, requiredOrder);

    /* update counters */
    b->freePages -= (1ULL << requiredOrder);

    /* return physical pointer */
    return (void *)block_phys;
}

void buddyFree(struct Zone *zone, void *vaddr) {
    if (!zone || !zone->buddy || !vaddr) return;
    uintptr_t phys = (uintptr_t)vaddr;
    struct Buddy *b = zone->buddy;

    if (phys < b->base || phys >= b->base + b->length) {
        // out of buddy range: ignore / error
        return;
    }

    size_t pageIndex = physToPageIndex(b, phys);
    uint8_t order = b->pageOrders[pageIndex];
    if (order >= BUDDY_MAX_ORDER) {
        // invalid order: ignore
        return;
    }

    /* Attempt to coalesce upward */
    size_t idx = pageIndex;
    size_t currentOrder = order;

    while (currentOrder < BUDDY_MAX_ORDER - 1) {
        size_t buddyIndex = idx ^ (1U << currentOrder);
        if (buddyIndex >= b->totalPages) break;

        /* buddy must be free and at same order: check pageOrders */
        if (b->pageOrders[buddyIndex] != currentOrder) break;

        /* buddy looks free at same order -> remove it from free-list */
        uintptr_t buddyPhys = buddyBlockPhys(b, buddyIndex);
        bool removed = removeFreeBlock(b, currentOrder, buddyPhys);
        if (!removed) {
            // couldn't find buddy in free list (so not free) -> stop coalescing
            break;
        }

        /* mark both halves' pageOrders to higher order (we'll update after loop) */
        if (buddyIndex < idx) idx = buddyIndex;
        currentOrder++;
        // continue loop to attempt bigger coalesce
    }

    /* Now idx is the start pageIndex of the merged block; currentOrder is the merged order */
    setPageOrders(b, idx, currentOrder);

    /* insert merged block into free list */
    uintptr_t mergedPhys = buddyBlockPhys(b, idx);
    addFreeBlock(b, currentOrder, mergedPhys);
    b->freePages += (1ULL << currentOrder);
}

void buddyDump(struct Buddy *b) {
    if (!b) {
        printf("Buddy = NULL\n");
        return;
    }

    printf("===== BUDDY ALLOCATOR DUMP =====\n");

    printf("Base          : 0x%lx\n", (uintptr_t)b->base);
    printf("Length        : %lu bytes\n", b->length);
    printf("Total Pages   : %lu\n", b->totalPages);
    printf("Free Pages    : %lu\n", b->freePages);
    printf("Max Order     : %lu (2^order pages)\n", BUDDY_MAX_ORDER - 1);

    printf("\n-- Free lists per order --\n");
    for (size_t order = 0; order < BUDDY_MAX_ORDER; order++) {
        struct FreeBlock *head = b->freeLists[order].head;
        uintptr_t head_phys = 0;
        if (head)
            head_phys = hhdmRemoveAddr((uintptr_t)head);

        printf("Order %lu | count=%lu | head_phys=0x%lx | head_virt=0x%lx\n",
               order,
               b->freeLists[order].count,
               (unsigned long)head_phys,
               (unsigned long)head);
    }

    printf("\n-- PageOrders summary (first few, last few) --\n");
    size_t show = (b->totalPages < 32) ? b->totalPages : 32;

    printf("First %lu pageOrders:\n", show);
    for (size_t i = 0; i < show; i++) {
        printf("%lu ", b->pageOrders[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    printf("Last %lu pageOrders:\n", show);
    for (size_t i = b->totalPages - show; i < b->totalPages; i++) {
        printf("%lu ", b->pageOrders[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    printf("===== END OF BUDDY DUMP =====\n\n");
}
