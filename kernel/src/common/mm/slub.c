#include <stdint.h>

#include <cpu/topology.h>
#include <mm/hhdm.h>
#include <mm/pmm.h>
#include <mm/zone.h>
#include <printf.h>

/* Configuration */
#define SLUB_PAGE_SIZE 4096U
#define SLUB_MIN_OBJ    8U         // minimum object size (bytes)
#define SLUB_MAX_OBJ    (SLUB_PAGE_SIZE / 2) // above this -> allocate whole pages
#define SLUB_ALIGN(x,a) (((x) + ((a)-1)) & ~((a)-1))

/* Size classes: powers of two starting at 8 up to SLUB_MAX_OBJ (inclusive) */
static const size_t slubSizeClasses[] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048
};
#define SLUB_SIZECLASS_COUNT (sizeof(slubSizeClasses)/sizeof(slubSizeClasses[0]))

/* === Metadata stored at start of each slab page (kept in physical memory at page base) ===
   We will place the SlubPageHeader at the start of a page (physical paddr).
   To modify/read it we map the page via hhdmAddAddr(paddr) to a virtual pointer.
*/
typedef struct SlubPageHeader {
    uint32_t magic;            // for sanity checking
    uint32_t objSize;          // size of objects on this page (bytes)
    uint32_t totalObjects;     // how many objects fit (computed)
    uint32_t freeCount;        // current free count
    uintptr_t nextPagePaddr;   // physical address of next Slub page for this cache (0 if none)
    uintptr_t freelistPaddr;   // physical address of first free object in this page (0 if none)
    // padding to make header small and aligned
} SlubPageHeader;

#define SLUB_MAGIC 0x53504C55 /* 'SPLU' */

/* A SlubCache per size-class */
typedef struct SlubCache {
    size_t objSize;
    uintptr_t pageListPaddr; // linked list of pages (physical addresses)
} SlubCache;

/* Global caches */
static SlubCache gSlubCaches[SLUB_SIZECLASS_COUNT];
static bool gSlubInitialized = false;

/* Helper: select size-class index for requested size */
static int slubSizeToIndex(size_t size) {
    size_t need = size;
    if (need < SLUB_MIN_OBJ) need = SLUB_MIN_OBJ;
    for (int i = 0; i < (int)SLUB_SIZECLASS_COUNT; ++i) {
        if (need <= slubSizeClasses[i]) return i;
    }
    return -1;
}

/* Helper: allocate a new page (physical paddr) for a given cache and initialize header+freelist */
static uintptr_t slubAllocNewPageForCache(SlubCache *cache) {
    // request one page from zone normal
    uintptr_t pagePaddr = (uintptr_t) pageAlloc(ZONE_NORMAL, 1);
    if (pagePaddr == 0) {
        printf("slub: pageAlloc failed for size %zu\n", cache->objSize);
        return 0;
    }

    // map to virtual to initialize
    void *v = (void *) hhdmAddAddr(pagePaddr);
    if (!v) {
        printf("slub: hhdmAddAddr failed for pagePaddr %p\n", (void *)pagePaddr);
        return 0;
    }

    SlubPageHeader *hdr = (SlubPageHeader *) v;
    hdr->magic = SLUB_MAGIC;
    hdr->objSize = (uint32_t) cache->objSize;

    // header consumes some bytes; we align object area after header to object's alignment
    size_t headerSize = SLUB_ALIGN(sizeof(SlubPageHeader), 8);
    size_t usable = SLUB_PAGE_SIZE - headerSize;
    size_t objSize = SLUB_ALIGN(cache->objSize, 8);
    uint32_t totalObjects = (uint32_t)(usable / objSize);
    hdr->totalObjects = totalObjects;
    hdr->freeCount = totalObjects;
    hdr->nextPagePaddr = cache->pageListPaddr;
    hdr->freelistPaddr = 0;

    // build freelist (store physical addresses of each object in the object memory)
    uintptr_t baseObjPaddr = pagePaddr + headerSize;
    uintptr_t prevObjPaddr = 0;
    for (uint32_t i = 0; i < totalObjects; ++i) {
        uintptr_t objPaddr = baseObjPaddr + (uintptr_t)i * objSize;
        // store next pointer (physical) at start of object memory
        uintptr_t *slot = (uintptr_t *)((uintptr_t)v + headerSize + (uintptr_t)i * objSize);
        // store as physical addresses, but since we're writing in mapped virtual memory,
        // compute next object's physical address
        *slot = prevObjPaddr; // we push onto a stack to create freelist
        prevObjPaddr = objPaddr;
    }
    hdr->freelistPaddr = prevObjPaddr;

    // Link this page into cache
    cache->pageListPaddr = pagePaddr;

#ifndef NDEBUG
    printfDebug("slub: new page 0x%lx for objSize %u total %u freelist 0x%lx\n",
                (unsigned long)pagePaddr, hdr->objSize, hdr->totalObjects, (unsigned long)hdr->freelistPaddr);
#endif

    // done (keep page mapped; it's okay if remains mapped for now)
    // Note: we don't unmap the page because we may need to touch it later; if desired we could hhdmRemoveAddr.
    return pagePaddr;
}

/* Initialize global caches */
void slubInit(void) {
    if (gSlubInitialized) return;
    for (size_t i = 0; i < SLUB_SIZECLASS_COUNT; ++i) {
        gSlubCaches[i].objSize = slubSizeClasses[i];
        gSlubCaches[i].pageListPaddr = 0;
    }
    gSlubInitialized = true;
#ifndef NDEBUG
    printfDebug("slub: initialized %zu size classes\n", (size_t)SLUB_SIZECLASS_COUNT);
#endif
}

/* Allocate: returns physical address as void* */
void *slubAlloc(size_t size) {
    if (!gSlubInitialized) slubInit();
    if (size == 0) size = 1;

    // If too large for slab, allocate whole pages and return page base paddr
    if (size > SLUB_MAX_OBJ) {
        // compute page count
        size_t pagesNeeded = (size + SLUB_PAGE_SIZE - 1) / SLUB_PAGE_SIZE;
        uintptr_t paddr = (uintptr_t) pageAlloc(ZONE_NORMAL, (size_t)pagesNeeded);
        if (paddr == 0) {
            printf("slub: large alloc pageAlloc failed for size %zu\n", size);
            return NULL;
        }
#ifndef NDEBUG
        printfDebug("slub: large alloc size %zu -> paddr 0x%lx (%zu pages)\n",
                    size, (unsigned long)paddr, pagesNeeded);
#endif
        return (void *) paddr;
    }

    int idx = slubSizeToIndex(size);
    if (idx < 0) {
        // shouldn't happen due to earlier check, but fallback to pageAlloc
        uintptr_t paddr = (uintptr_t) pageAlloc(ZONE_NORMAL, 1);
        return (void *) paddr;
    }

    SlubCache *cache = &gSlubCaches[idx];

    // find a page with a free object
    uintptr_t pagePaddr = cache->pageListPaddr;
    SlubPageHeader *hdr = NULL;
    void *pageVaddr = NULL;
    while (pagePaddr != 0) {
        pageVaddr = (void *) hhdmAddAddr(pagePaddr);
        if (!pageVaddr) {
            printf("slub: hhdmAddAddr failed for page 0x%lx while searching\n", (unsigned long)pagePaddr);
            return NULL;
        }
        hdr = (SlubPageHeader *) pageVaddr;
        if (hdr->magic != SLUB_MAGIC) {
            // corrupted page? skip it (but warn)
            printf("slub: warning: page 0x%lx has bad magic\n", (unsigned long)pagePaddr);
            pagePaddr = hdr->nextPagePaddr;
            continue;
        }
        if (hdr->freeCount > 0) break;
        // else move to next
        pagePaddr = hdr->nextPagePaddr;
        // leave mapped page mapped for now
    }

    // if none found, allocate a new page and init freelist
    if (pagePaddr == 0) {
        pagePaddr = slubAllocNewPageForCache(cache);
        if (pagePaddr == 0) {
            return NULL;
        }
        pageVaddr = (void *) hhdmAddAddr(pagePaddr);
        hdr = (SlubPageHeader *) pageVaddr;
    }

    // Pop one object from this page's freelist
    uintptr_t objPaddr = hdr->freelistPaddr;
    if (objPaddr == 0) {
        // inconsistent: freeCount > 0 but freelist empty
        printf("slub: inconsistent freelist on page 0x%lx\n", (unsigned long)pagePaddr);
        return NULL;
    }

    // To get the next pointer stored inside the object, map object virtual
    uintptr_t objVaddr = (uintptr_t) hhdmAddAddr(objPaddr);
    if (!objVaddr) {
        printf("slub: hhdmAddAddr failed for objPaddr 0x%lx\n", (unsigned long)objPaddr);
        return NULL;
    }
    // next stored at object start
    uintptr_t nextObjPaddr = *((uintptr_t *) objVaddr);

    // update header
    hdr->freelistPaddr = nextObjPaddr;
    hdr->freeCount -= 1;

#ifndef NDEBUG
    printfDebug("slub: alloc obj 0x%lx (page 0x%lx) size %u freeLeft %u\n",
                (unsigned long)objPaddr, (unsigned long)pagePaddr, hdr->objSize, hdr->freeCount);
#endif

    // return physical address as void*
    return (void *) objPaddr;
}

/* Free: accepts paddr (physical) */
void slubFree(void *paddr) {
    if (paddr == NULL) return;
    uintptr_t objPaddr = (uintptr_t) paddr;

    // find page base (align down to SLUB_PAGE_SIZE)
    uintptr_t pagePaddr = objPaddr & ~(SLUB_PAGE_SIZE - 1);

    // map page
    void *pageVaddr = (void *) hhdmAddAddr(pagePaddr);
    if (!pageVaddr) {
        printf("slub: hhdmAddAddr failed for pagePaddr 0x%lx in free\n", (unsigned long)pagePaddr);
        return;
    }
    SlubPageHeader *hdr = (SlubPageHeader *) pageVaddr;
    if (hdr->magic != SLUB_MAGIC) {
        // Might be a large-allocation (page-based) where objPaddr == pagePaddr
        // In our design we returned page base for large allocs, so free can't free page (not implemented)
#ifndef NDEBUG
        printfDebug("slub: free called on non-slab page 0x%lx (magic=0x%x) - ignoring\n",
                    (unsigned long)pagePaddr, hdr->magic);
#endif
        // We don't support freeing whole-page allocations (page free not implemented).
        return;
    }

    // push the object into the page freelist
    uintptr_t objVaddr = (uintptr_t) hhdmAddAddr(objPaddr);
    if (!objVaddr) {
        printf("slub: hhdmAddAddr failed for objPaddr 0x%lx in free\n", (unsigned long)objPaddr);
        return;
    }

    // store current freelist head into object start
    *((uintptr_t *) objVaddr) = hdr->freelistPaddr;
    // update header
    hdr->freelistPaddr = objPaddr;
    hdr->freeCount += 1;

#ifndef NDEBUG
    printfDebug("slub: free obj 0x%lx (page 0x%lx) freeCount %u/%u\n",
                (unsigned long)objPaddr, (unsigned long)pagePaddr,
                hdr->freeCount, hdr->totalObjects);
#endif

    // Note: we don't free pages even if all objects are free since no pageFree exists
}

void slubDumpStats() {
    if (!gSlubInitialized) {
        printfDebug("slub: not initialized yet\n");
        return;
    }

    printfDebug("==== SLUB STATS DUMP BEGIN ====\n");

    for (size_t i = 0; i < SLUB_SIZECLASS_COUNT; i++) {
        SlubCache *cache = &gSlubCaches[i];

        if (cache->objSize == 0)
            continue; // unused size class

        printfDebug("Class[%lu]: objSize=%lu\n", (unsigned)i, (unsigned)cache->objSize);

        uintptr_t pagePaddr = cache->pageListPaddr;
        uint64_t pageCount = 0;
        uint64_t totalObjs = 0;
        uint64_t totalFree = 0;

        while (pagePaddr != 0) {
            pageCount++;

            SlubPageHeader *hdr = (SlubPageHeader*)hhdmAddAddr(pagePaddr);

#ifndef NDEBUG
            if (hdr->magic != SLUB_MAGIC) {
                printfDebug("  !! BAD PAGE MAGIC at 0x%lx\n", pagePaddr);
                break;
            }
#endif

            totalObjs += hdr->totalObjects;
            totalFree += hdr->freeCount;

            printfDebug("  page 0x%lx: objects=%lu free=%lu\n",
                        pagePaddr,
                        hdr->totalObjects,
                        hdr->freeCount);

            pagePaddr = hdr->nextPagePaddr;
        }

        printfDebug("  => pages=%lu, objects=%lu, free=%lu\n",
                    pageCount, totalObjs, totalFree);
    }

    printfDebug("==== SLUB STATS DUMP END ====\n");
}