#include <macros.h>
#include <format_string.h>
#include <serial.h>
#include <mm/buddy_allocator.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/memmap.h>
#include <mm/zone.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define THRESHOLD_16MIB 1024 * 1024 * 16L
#define THRESHOLD_4GIB 1024 * 1024 * 1024 * 4L

static struct Zone *zoneTypeCache[ZONE_TYPE_MAX];
static struct Zone *lastZoneByAddr = NULL;

static struct Zone *zones;
static size_t zoneCount;

static void adjustZoneSize(struct Zone *zone, struct Zone *prevZone, size_t threshold, enum ZoneType type);

void pmmInit() {
    assert(memmap.entryTotalCount > 0);
    assert(memmap.usable != NULL);
    assert(memmap.usable->count > 0);
    
    serialPuts("[pmm] Initializing Page Allocator\r\n");

    // Misc Variables
    char formatBufs[16];
    uintptr_t buddyAddr;
    uintptr_t alignedAddr;
    uint32_t i, j;
    
    // Zone Variables
    const size_t zoneMetadataSize = (memmap.usable->count + 1) * sizeof(struct Zone);
    struct Zone *zone;
    struct Zone *prevZone = NULL;

    // Buddy Variables
    size_t buddyMetadataSize[memmap.usable->count];
    struct Buddy *buddy;
    struct FreeBlock *freeBlock = NULL;

    // Entry Variables
    struct MemoryMapEntry *metadataEntry    = NULL;
    struct MemoryMapEntry *entry            = NULL;
    struct MemoryMapEntry *usableEntries    = memmap.usable->entries;
    
    // First, get ourself some entry that are big enough to fit the metadata
    for (i = 0; i < memmap.usable->count; i++) {
        entry = usableEntries + i;

        if (entry->length >= zoneMetadataSize && (metadataEntry == NULL || entry->length < metadataEntry->length)) {
            metadataEntry = entry;
        }
    }

    // No entry is big enough to fit the whole zone metadata in one big chunk so, THROW ERROR!
    if (!metadataEntry) {
        formatInteger(zoneMetadataSize, formatBufs, sizeof(formatBufs), 0);
        serialPuts("[pmm] No entry is big enough to fit zone metadata: ");
        serialPuts(formatBufs);
        serialPuts(" bytes.\r\n[sys] Aborting.\r\n");
        hang();
    }
    
    // Align metadata for quicker access
    alignedAddr             = __alignup(metadataEntry->base, 64);
    zones                   = (struct Zone *) hhdmAdd((void *) alignedAddr);
    zoneCount               = memmap.usable->count;
    metadataEntry->length  -= alignedAddr - metadataEntry->base;
    metadataEntry->base     = alignedAddr + zoneMetadataSize;

    // Adjust zones range to match threshold better
    for (i = 0; i < zoneCount; ++i) {
        zone            = zones + i;
        zone->base      = usableEntries[i].base;
        zone->length    = usableEntries[i].length;

        if (zone->base < THRESHOLD_16MIB) {
            adjustZoneSize(zone, prevZone, THRESHOLD_16MIB, ZONE_DMA | ZONE_DMA32);
        }
    #ifdef ARCH_64
        else if (zone->base < THRESHOLD_4GIB) {
            adjustZoneSize(zone, prevZone, THRESHOLD_4GIB, ZONE_DMA32 | ZONE_NORMAL);
        }
    #else
        else if (zone->base > THRESHOLD_4GIB) {
            _setZoneType(zone, prevZone, THRESHOLD_4GIB, ZONE_HIGHMEM | ZONE_NORMAL);
        }
    #endif
        else {
            zone->type = ZONE_NORMAL;
        }

        prevZone = zone;
        buddyMetadataSize[i] = sizeof(struct Buddy) + (zone->length / PAGE_SIZE);
    }

    serialPuts("[pmm] Initialized Zone-style Page Allocator\r\n");
    serialPuts("[pmm] Initializing Buddy Allocator\r\n");

    // Initialize each zone's buddy allocator metadata
    for (i = 0; i < zoneCount; ++i) {

        // If currently used memory entry is too small to fit the metadata, then search for another entry
        if (metadataEntry->length < buddyMetadataSize[i]) {
            metadataEntry =  NULL;
            for (j = 0; j < memmap.usable->count; j++) {
                entry = usableEntries + j;
                if (entry->length >= buddyMetadataSize[i] && (metadataEntry == NULL || entry->length < metadataEntry->length)) {
                    metadataEntry = entry;
                }
            }
        }

        if (metadataEntry == NULL) {
            formatInteger(i, formatBufs, sizeof(formatBufs), 0);
            serialPuts("[pmm] No entry is big enough to fit buddy no. ");
            serialPuts(formatBufs);
            serialPuts(" metadata:\r\n");
            
            formatInteger(buddyMetadataSize[i], formatBufs, sizeof(formatBufs), 0);
            serialPuts(formatBufs);
            serialPuts("      bytes.\r\n [sys] Aborting\r\n");
            hang();
        }
        
                           zone = zones + i;
                    alignedAddr = __alignup(metadataEntry->base, 64);
                      buddyAddr = (uintptr_t) hhdmAdd((void *) alignedAddr);
                    zone->buddy = (struct Buddy *) buddyAddr;
         metadataEntry->length -= alignedAddr - metadataEntry->base;
           metadataEntry->base  = alignedAddr + buddyMetadataSize[i];

        buddy                   = zone->buddy;
        buddy->freePages        =
        buddy->totalPages       = zone->length / PAGE_SIZE;

        // Nullify the buddy free lists item, no need to init the array itself too since it's statically allocated
        for (j = 0; j < BUDDY_MAX_ORDER; ++j) {
            buddy->freeLists[j].count = 0;
            buddy->freeLists[j].head  = NULL;
        }

        buddy->pageOrders = (uint8_t *) (buddyAddr + sizeof(struct Buddy));
        for (j = 0; j < buddy->totalPages; ++j) {
            buddy->pageOrders[j] = BUDDY_MAX_ORDER - 1;
        }

        freeBlock = (struct FreeBlock *) hhdmAdd((void *) zone->base);
        freeBlock->next = NULL;
        buddy->freeLists[BUDDY_MAX_ORDER - 1].head = freeBlock;
    }

    serialPuts("[pmm] Initialized Buddy Allocator\r\n");
}

static void adjustZoneSize(struct Zone *zone, struct Zone *prevZone, size_t threshold, enum ZoneType type) {
    zone->type = type;
    if (prevZone == NULL) {
        return;
    }

    size_t range = prevZone->base + prevZone->length;
    if (range > threshold && range + 1 == zone->base) {
        uintptr_t diff = (prevZone->base + prevZone->length) - threshold;
        prevZone->length -= diff;
        zone->base = 0;
        zone->length += diff;
    }
}

static inline struct Zone *findZoneByType(size_t size, uint8_t type) {
    // Find index bit
    int bitIndex = __builtin_ctz(type); // get lowest active bit
    struct Zone *cached = zoneTypeCache[bitIndex];

    // Scenario 1: Cache hit and is valid
    if (cached && (cached->type & type) && cached->length >= size)
        return cached;

    // Scenario 2: Cache miss → scan all zone
    struct Zone *best = NULL;
    for (size_t i = 0; i < zoneCount; i++) {
        struct Zone *z = &zones[i];
        if ((z->type & type) && z->length >= size) {
            if (!best || z->length < best->length)
                best = z;
        }
    }

    // Save zone to cache for faster next allocation
    if (best)
        zoneTypeCache[bitIndex] = best;

    return best;
}

static inline struct Zone *findZoneByAddress(uintptr_t addr) {
    if (lastZoneByAddr &&
        addr >= lastZoneByAddr->base &&
        addr < lastZoneByAddr->base + lastZoneByAddr->length)
        return lastZoneByAddr;
    
    size_t low = 0, high = zoneCount - 1;
    while (low <= high) {
        size_t mid = (low + high) / 2;
        struct Zone *z = &zones[mid];

        if (addr < z->base)
            high = mid - 1;
        else if (addr >= z->base + z->length)
            low = mid + 1;
        else
            return z; // Found zone
    }
    return NULL;
}

void *pageAllocAligned(enum ZoneType type, size_t pageCount, size_t alignment) {
    struct Zone *zone = findZoneByType(pageCount * PAGE_SIZE, type);
    if (!zone) return NULL;

    size_t order = 0, pages = 1;
    while (pages < pageCount) { pages <<= 1; ++order; }

    #ifndef NDEBUG
        zone->stats.allocCount++;
        zone->stats.pagesAllocated += pages;
        zone->stats.lastAllocOrder = order;
    #endif

    return buddyAllocAligned(zone, order, alignment);
}

void *pageAlloc(enum ZoneType type, size_t pageCount) {
    return pageAllocAligned(type, pageCount, 0);
}

void pageFree(void *addr) {
    struct Zone *zone = findZoneByAddress((uintptr_t) addr);
    if (!zone) {
        return;
    }

    #ifndef NDEBUG
        zone->stats.freeCount++;
    #endif

    buddyFree(zone, addr);
}

#ifndef NDEBUG
void pmmDumpStats(void) {
    // serialPuts("=== Page Allocator Stats ===\r\n");
    // for (size_t i = 0; i < zoneCount; i++) {
    //     struct Zone *z = &zones[i];
    //     kprintinfo(
    //         "Zone[%ld]: base=%p, len=%ld, type=0x%02X | alloc=%ld, free=%ld, pages=%ld\n",
    //         i, (void*)z->base, z->length, z->type,
    //         z->stats.allocCount, z->stats.freeCount,
    //         z->stats.pagesAllocated
    //     );
    // }
}
#else
void pmmDumpStats(void) {}
#endif