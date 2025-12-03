#include "serial.h"
#include <limine/deploy.h>
#include <limine/limine.h>

#include <macros.h>
#include <mm/memmap.h>
#include <stddef.h>

LIMINE_GET_MEMMAP();

#define PER_TYPE_ENTRY_COUNT 12
#define TYPE_COUNT 7

static struct MemoryMapEntry framebufferEntry __bootdata;
static struct MemoryMapEntry kernelEntry __bootdata;

static struct MemoryMapEntry usableEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry
    bootloaderReclaimableEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry
    acpiReclaimableEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry acpiNvsEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry acpiTableEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry mmioEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry reservedEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry badmemEntries[PER_TYPE_ENTRY_COUNT] __bootdata;
static struct MemoryMapEntry unknownEntries[PER_TYPE_ENTRY_COUNT] __bootdata;

static struct MemoryEntries usable;
static struct MemoryEntries bootloaderReclaimable;
static struct MemoryEntries acpiReclaimable;
static struct MemoryEntries acpiNvs;
static struct MemoryEntries acpiTable;
static struct MemoryEntries mmio;
static struct MemoryEntries reserved;
static struct MemoryEntries badmem;
static struct MemoryEntries unknown;

void abstractMemmap() {
  size_t totalCount = 0;
  size_t usableCount = 0;
  size_t bootloaderReclaimableCount = 0;
  size_t acpiReclaimableCount = 0;
  size_t acpiNvsCount = 0;
  size_t acpiTableCount = 0;
  size_t reservedCount = 0;
  size_t badmemCount = 0;
  size_t unknownCount = 0;

  struct limine_memmap_response res = *LIMINE_REQ(memmap).response;
  struct limine_memmap_entry *entry;
  struct MemoryMapEntry *entryOut;

  serialPuts("start abstracting memmap, looping!\r\n");

  for (size_t i = 0;
       i < res.entry_count && i < (PER_TYPE_ENTRY_COUNT * TYPE_COUNT); i++) {
    serialPuts("entering entry ");
    if (i >= 10) {
      serialPutc('0' + (i / 10));
      serialPutc('0' + (i % 10));
    } else {
      serialPutc('0' + i);
    }
    serialPuts("\r\n");

    entry = res.entries[i];
    entryOut = NULL;
    totalCount++;
    switch (entry->type) {
    case LIMINE_MEMMAP_USABLE:
      if (usableCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = usableEntries + usableCount++;
      break;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
      if (bootloaderReclaimableCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = bootloaderReclaimableEntries + bootloaderReclaimableCount++;
      break;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
      if (acpiReclaimableCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = acpiReclaimableEntries + acpiReclaimableCount++;
      break;
    case LIMINE_MEMMAP_ACPI_NVS:
      if (acpiNvsCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = acpiNvsEntries + acpiNvsCount++;
      break;
    case LIMINE_MEMMAP_ACPI_TABLES:
      if (acpiTableCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = acpiTableEntries + acpiTableCount++;
      break;
    case LIMINE_MEMMAP_FRAMEBUFFER:
      entryOut = &framebufferEntry;
      break;
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
      entryOut = &kernelEntry;
      break;
    case LIMINE_MEMMAP_RESERVED:
      if (reservedCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = reservedEntries + reservedCount++;
      break;
    case LIMINE_MEMMAP_BAD_MEMORY:
      if (badmemCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = badmemEntries + badmemCount++;
      break;
    default:
      if (unknownCount >= PER_TYPE_ENTRY_COUNT)
        break;
      entryOut = unknownEntries + unknownCount++;
      break;
    }

    if(entryOut) {
      entryOut->base = entry->base;
      entryOut->length = entry->length;
    }
  }

  usable.entries = usableEntries;
  usable.count = usableCount;
  bootloaderReclaimable.entries = bootloaderReclaimableEntries;
  bootloaderReclaimable.count = bootloaderReclaimableCount;
  acpiReclaimable.entries = acpiReclaimableEntries;
  acpiReclaimable.count = acpiReclaimableCount;
  acpiNvs.entries = acpiNvsEntries;
  acpiNvs.count = acpiNvsCount;
  acpiTable.entries = acpiTableEntries;
  acpiTable.count = acpiTableCount;
  mmio.entries = mmioEntries;
  mmio.count = 0;
  reserved.entries = reservedEntries;
  reserved.count = reservedCount;
  badmem.entries = badmemEntries;
  badmem.count = badmemCount;
  unknown.entries = unknownEntries;
  unknown.count = unknownCount;

  memmap.entryTotalCount = totalCount;
  memmap.kernel = &kernelEntry;
  memmap.framebuffer = &framebufferEntry;
  memmap.bootloaderReclaimable = &bootloaderReclaimable;
  memmap.acpiReclaimable = &acpiReclaimable;
  memmap.acpiNvs = &acpiNvs;
  memmap.acpiTable = &acpiTable;
  memmap.mmio = &mmio;
  memmap.reserved = &reserved;
  memmap.badmem = &badmem;
  memmap.unknown = &unknown;
}