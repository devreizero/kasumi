#ifndef MEMMAP_H
#define MEMMAP_H

#include <stddef.h>
#include <stdint.h>

enum MemoryType {
    MEM_UNKNOWN               ,
    MEM_USABLE                ,
    MEM_BOOTLOADER_RECLAIMABLE,
    MEM_ACPI_RECLAIMABLE      ,
    MEM_ACPI_NVS              ,
    MEM_ACPI_TABLE            ,
    MEM_FRAMEBUFFER           ,
    MEM_MMIO                  ,
    MEM_KERNEL                ,
    MEM_RESERVED              ,
    MEM_BADMEM                ,
};

struct MemoryMapEntry {
    uintptr_t base;
    size_t    length;
};

struct MemoryEntries {
    struct MemoryMapEntry *entries;
    size_t count;
};

struct MemoryMap {
    struct MemoryMapEntry *framebuffer;
    struct MemoryMapEntry *kernel;

    struct MemoryEntries *usable;
    struct MemoryEntries *bootloaderReclaimable;
    struct MemoryEntries *acpiReclaimable;
    struct MemoryEntries *acpiNvs;
    struct MemoryEntries *acpiTable;
    struct MemoryEntries *mmio;
    struct MemoryEntries *reserved;
    struct MemoryEntries *badmem;
    struct MemoryEntries *unknown;

    size_t entryTotalCount;
};

void dumpMemmap();
void abstractMemmap();

extern struct MemoryMap memmap;

#endif