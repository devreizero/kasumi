#pragma once
#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#define BUDDY_MAX_ORDER 11

struct Zone;

struct FreeList {
    struct FreeBlock *head;
    size_t count;
};

struct FreeBlock {
    struct FreeBlock *next;
};

struct Buddy {
    struct FreeList freeLists[BUDDY_MAX_ORDER];
    uint8_t *pageOrders;
    size_t totalPages;
    size_t freePages;
};

void *buddyAlloc(struct Zone *zone, size_t order);
void *buddyAllocAligned(struct Zone *zone, size_t order, size_t align);
void buddyFree(struct Zone *self, void *vaddr);

#endif