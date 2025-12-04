#pragma once
#ifndef ZONE_H
#define ZONE_H

#include <stdint.h>
#include <stddef.h>

#define ZONE_TYPE_MAX 8

enum ZoneType {
    ZONE_DMA        = 1 << 0,
    ZONE_DMA32      = 1 << 1,
    ZONE_HIGHMEM    = 1 << 2,
    ZONE_NORMAL     = 1 << 3,
    ZONE_MOVABLE    = 1 << 4,
    ZONE_SYSTEM     = 1 << 7
};

#ifndef NDEBUG
struct ZoneStats {
    size_t allocCount;     // total pageAlloc() calls
    size_t freeCount;      // total pageFree() calls
    size_t pagesAllocated; // total allocated pages
    size_t pagesFreed;     // total freed pages
    size_t lastAllocOrder; // last allocated order
};
#endif

struct Zone {
    #ifndef NDEBUG
    struct ZoneStats stats;
    #endif

    struct Buddy *buddy;
    uintptr_t base;
    size_t length;
    uint8_t type;
};

#endif