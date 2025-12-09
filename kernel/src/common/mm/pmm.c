#include <assert.h>
#include <macros.h>
#include <mm/buddy_allocator.h>
#include <mm/hhdm.h>
#include <mm/kmap.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <mm/zone.h>
#include <panic.h>
#include <printf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdmem.h>

#define THRESHOLD_16MIB 1024 * 1024 * 16L
#define THRESHOLD_4GIB 1024 * 1024 * 1024 * 4L

static struct Zone *zoneTypeCache[ZONE_TYPE_MAX];
static struct Zone *lastZoneByAddr = NULL;

static struct Zone *zones;
static size_t zoneCount;

static uint8_t pickZoneType(uintptr_t base);
static struct MemoryMapEntry *findSmallestUsableEntry(size_t need);
static void initBuddyForZone(struct Zone *z);
static inline struct Zone *findZoneByType(size_t size, uint8_t type);
static inline struct Zone *findZoneByAddress(uintptr_t addr);

__init void pmmInit() {
  assert(memmap.usable && memmap.usable->count > 0);

  printfInfo("pmm: Initializing Page Allocator\n");

  size_t usableCount = memmap.usable->count;
  struct MemoryMapEntry *usable = memmap.usable->entries;

  /* --- Step 1: Allocate Zone Metadata --- */
  size_t zoneMetaSize = usableCount * sizeof(struct Zone);

  struct MemoryMapEntry *metaEntry = findSmallestUsableEntry(zoneMetaSize);
  if (!metaEntry) {
    panic("pmm: Unable to reserve %lu bytes for zone metadata\n", zoneMetaSize);
  }

  uintptr_t metaPhys = __alignup(metaEntry->base, 64);
  zones = (struct Zone *)hhdmAdd((void *)metaPhys);
  zoneCount = usableCount;

  /* shrink entry because metadata occupies part of it */
  size_t consumed = metaPhys - metaEntry->base + zoneMetaSize;
  metaEntry->base += consumed;
  metaEntry->length -= consumed;

  /* add metadata entry to memmap */
  memmap.allocator->base = metaEntry->base - consumed;
  memmap.allocator->length = consumed;

  /* --- Step 2: Fill zone table --- */
  for (size_t i = 0; i < usableCount; i++) {
    zones[i].base = usable[i].base;
    zones[i].length = usable[i].length;
    zones[i].type = pickZoneType(zones[i].base);

#ifndef NDEBUG
    memset(&zones[i].stats, 0, sizeof(struct ZoneStats));
#endif
  }

  printfOk("pmm: Zones initialized\n");
  printfInfo("pmm: Initializing Buddy allocators...\n");

  /* --- Step 3: Build buddy metadata for every zone --- */
  for (size_t i = 0; i < usableCount; i++) {
    struct Zone *z = &zones[i];

    size_t metaSize = sizeof(struct Buddy);
    metaSize += (z->length / PAGE_SIZE); // pageOrders array

    struct MemoryMapEntry *entry = findSmallestUsableEntry(metaSize);
    if (!entry) {
      panic("pmm: Failed to allocate buddy metadata for zone %lu (%lu bytes)\n",
            i, metaSize);
    }

    uintptr_t phys = __alignup(entry->base, 64);
    z->buddy = (struct Buddy *)hhdmAdd((void *)phys);

    size_t used = phys - entry->base + metaSize;
    entry->base += used;
    entry->length -= used;

    initBuddyForZone(z);
  }

  printfOk("pmm: All Buddy allocators initialized.\n");
}

__init void pmmMap() {
  struct Zone *z;
  struct Buddy *b;
  uintptr_t zoneAligned, buddyAligned, pageOrdersAligned,
      previousZoneAlignedAddr = 0;
  for (size_t i = 0; i < zoneCount; i++) {
    z = &zones[i];
    b = z->buddy;

    zoneAligned = __aligndown((uintptr_t)z, ALIGN_4KB);
    buddyAligned = __aligndown((uintptr_t)b, ALIGN_4KB);
    pageOrdersAligned = __aligndown((uintptr_t)b->pageOrders, ALIGN_4KB);

    if (zoneAligned != previousZoneAlignedAddr) {
      kmap(hhdmRemoveAddr((uintptr_t)zoneAligned), (uintptr_t)zoneAligned,
           SIZE_4KB);
    }

    if (buddyAligned != zoneAligned) {
      kmap(hhdmRemoveAddr((uintptr_t)buddyAligned), (uintptr_t)buddyAligned,
           SIZE_4KB);
    }

    if (b->pageOrders) {
      size_t pageOrdersSize = b->totalPages * sizeof(uint8_t);
      size_t pagesToMap = __alignup(pageOrdersSize, PAGE_SIZE) / PAGE_SIZE;

      kmap(hhdmRemoveAddr((uintptr_t)pageOrdersAligned),
           (uintptr_t)pageOrdersAligned, pagesToMap * SIZE_4KB);
    }

    previousZoneAlignedAddr = zoneAligned;
  }
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init Helper
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static struct MemoryMapEntry *findSmallestUsableEntry(size_t need) {
  struct MemoryMapEntry *best = NULL;

  for (size_t i = 0; i < memmap.usable->count; i++) {
    struct MemoryMapEntry *e = &memmap.usable->entries[i];
    if (e->length >= need) {
      if (!best || e->length < best->length)
        best = e;
    }
  }
  return best;
}

static uint8_t pickZoneType(uintptr_t base) {
  if (base < 16ULL * 1024 * 1024)
    return ZONE_DMA | ZONE_DMA32;
  if (base < 4ULL * 1024 * 1024 * 1024)
    return ZONE_DMA32 | ZONE_NORMAL;
  return ZONE_NORMAL; // fallback
}

static void initBuddyForZone(struct Zone *z) {
  struct Buddy *b = z->buddy;

  uintptr_t zoneBase =
      __alignup(z->base, PAGE_SIZE); // make sure it is 4KB aligned
  uintptr_t zoneEnd = z->base + z->length;

  // calculate buddy's biggest block size
  size_t maxBlock = (1ULL << (BUDDY_MAX_ORDER + PAGE_SHIFT)); // default = 8 MiB

  // align down zoneEnd to maxBlock boundary
  uintptr_t alignedEnd = __aligndown(zoneEnd, maxBlock);

  // make sure there is no underflow and isn't forcing to leave the zone
  if (alignedEnd < zoneBase + maxBlock) {
    // zone is too small to contain the biggest block,
    // so we dynamically reduce maxBlock (drop the order)
    size_t dynamicOrder = BUDDY_MAX_ORDER - 1;
    maxBlock >>= 1;

    while (dynamicOrder > 0 && alignedEnd < zoneBase + maxBlock) {
      dynamicOrder--;
      maxBlock >>= 1;
    }

    // still not enough → the zone is really small
    if (alignedEnd < zoneBase + PAGE_SIZE) {
      // fallback: not large enough → mark as empty
      b->base = zoneBase;
      b->length = 0;
      b->totalPages = 0;
      b->freePages = 0;

      memset(b->freeLists, 0, sizeof(b->freeLists));
      return; // buddy allocator is empty but doesn't crash
    }
  }

  // now we have a valid buddy range
  b->base = zoneBase;
  b->length = alignedEnd - zoneBase;

  b->totalPages = b->length / PAGE_SIZE;
  b->freePages = b->totalPages;

  // free lists
  for (size_t i = 0; i < BUDDY_MAX_ORDER; i++) {
    b->freeLists[i].head = NULL;
    b->freeLists[i].count = 0;
  }

  // pageOrders array
  b->pageOrders = (uint8_t *)((uintptr_t)b + sizeof(struct Buddy));
  memset(b->pageOrders, BUDDY_MAX_ORDER - 1, b->totalPages);

  // insert one big block (at the buddy base, not at z->base which may not be
  // aligned)
  struct FreeBlock *blk = (struct FreeBlock *)hhdmAdd((void *)(b->base));
  blk->next = NULL;

  b->freeLists[BUDDY_MAX_ORDER - 1].head = blk;
  b->freeLists[BUDDY_MAX_ORDER - 1].count = 1;
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Core
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *pageAllocAligned(enum ZoneType type, size_t pageCount, size_t alignment) {
  size_t bytes = pageCount * PAGE_SIZE;

  struct Zone *z = findZoneByType(bytes, type);
  if (!z)
    return NULL;

  /* convert pageCount → order */
  size_t order = 0, pages = 1;
  while (pages < pageCount) {
    pages <<= 1;
    order++;
  }

#ifndef NDEBUG
  z->stats.allocCount++;
  z->stats.pagesAllocated += pages;
  z->stats.lastAllocOrder = order;
#endif

  void *buddyResult = buddyAllocAligned(z, order, alignment);
  return buddyResult;
}

void *pageAlloc(enum ZoneType type, size_t pageCount) {
  return pageAllocAligned(type, pageCount, PAGE_SIZE);
}

void pageFree(void *addr) {
  if (!addr)
    return;

  uintptr_t a = (uintptr_t)addr;
  struct Zone *z = findZoneByAddress(a);
  if (!z)
    return;

#ifndef NDEBUG
  z->stats.freeCount++;
  z->stats.pagesFreed++;
#endif

  buddyFree(z, addr);
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zone Finder
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct Zone *findZoneByType(size_t size, uint8_t type) {
  if (type == 0)
    return NULL;

  /* pages we need */
  size_t needed_pages = (__alignup(size, PAGE_SIZE) / PAGE_SIZE);
  if (needed_pages == 0)
    needed_pages = 1;

  int bitIndex = __builtin_ctz(type); // lowest active bit for cache slot
  struct Zone *cached = NULL;
  if (bitIndex >= 0 && bitIndex < ZONE_TYPE_MAX)
    cached = zoneTypeCache[bitIndex];

  if (cached && (cached->type & type) && cached->length > 0 &&
      cached->buddy != NULL && cached->buddy->freePages >= needed_pages) {
    return cached;
  }

  struct Zone *best = NULL;

  for (size_t i = 0; i < zoneCount; ++i) {
    struct Zone *z = &zones[i];
    if (!z)
      continue;
    // TOOD: Fix Type  --  if (!(z->type & type)) continue;
    if (z->length == 0)
      continue;
    if (!z->buddy)
      continue;
    if (z->buddy->freePages < needed_pages)
      continue;

    if (!best) {
      best = z;
      continue;
    }

    /* tie-breaker: prefer zone with more free pages (higher chance success) */
    if (z->buddy->freePages > best->buddy->freePages) {
      best = z;
      continue;
    }

    /* alternate tie-breaker (uncomment to prefer smaller buddy ranges):
       if (z->buddy->length < best->buddy->length) best = z;
    */
  }

  if (best && bitIndex >= 0 && bitIndex < ZONE_TYPE_MAX) {
    zoneTypeCache[bitIndex] = best;
  }

  return best;
}

static inline struct Zone *findZoneByAddress(uintptr_t addr) {
  if (lastZoneByAddr && addr >= lastZoneByAddr->base &&
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

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void pmmDumpStats(bool dumpBuddy) {
  printfInfo("=== Page Allocator Stats ===\n");
  for (size_t i = 0; i < zoneCount; i++) {
    struct Zone *z = &zones[i];
    printfInfo("Zone[%lu]: base=0x%lx, len=%lu, type=0x%x | alloc=%lu, "
               "free=%lu, pages=%lu\n",
               i, (void *)z->base, z->length, z->type, z->stats.allocCount,
               z->stats.freeCount, z->stats.pagesAllocated);
    if (dumpBuddy) {
      buddyDump(z->buddy);
    }
  }
}
#else
void pmmDumpStats(bool dumpBuddy) {}
#endif