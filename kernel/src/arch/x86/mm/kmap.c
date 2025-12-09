#include <basic-io.h>
#include <macros.h>
#include <mm/hhdm.h>
#include <mm/kmap.h>
#include <mm/pmm.h>
#include <mm/slub.h>
#include <mm/vmm.h>
#include <mm/zone.h>
#include <printf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdmem.h>
#include <x86/page_table.h>

extern size_t pagePhysicalMask;
extern size_t pagePhysicalBits;
extern size_t pageVirtualBits;

#define __allocpage() pageAlloc(ZONE_NORMAL, 1);

#ifdef ARCH_64
extern uint64_t *root;
#define PTE_P (1ULL << 0)
#define PTE_RW (1ULL << 1)
#define PTE_US (1ULL << 2)  // optional
#define PTE_PS (1ULL << 7)  // for large pages (not used here)
#define PTE_NX (1ULL << 63) // if NXE enabled

#else
#error "Only x86_64 supported in early VMM"
#endif

#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ULL

static bool mapSinglePage(uintptr_t paddr, uintptr_t vaddr, int targetLevel);
static bool getExistingEntry(uintptr_t *table, uintptr_t *output,
                             uintptr_t vaddr, int targetLevel);
static uintptr_t *makeNewEntry(uintptr_t *table, uintptr_t paddr,
                               uintptr_t vaddr, int targetLevel);

static inline int getIndex(uintptr_t vaddr, int level) {
  switch (level) {
  case 3:
    return (vaddr >> 39) & 0x1FF; // PML4 → PDPT
  case 2:
    return (vaddr >> 30) & 0x1FF; // PDPT → PD
  case 1:
    return (vaddr >> 21) & 0x1FF; // PD → PT
  case 0:
    return (vaddr >> 12) & 0x1FF; // PT → Frame
  }
  return -1; // error
}

void *kmap(uintptr_t paddr, uintptr_t vaddr, size_t size) {
  uintptr_t offset = 0;
  size_t remaining = size;

  printfDebug("@@ Mapping paddr 0x%lx until 0x%lx -> vaddr 0x%lx until 0x%lx\n",
              paddr, paddr + size, vaddr, vaddr + size);

  struct VMMNode *conflict = vmmFindNodeOverlapping(vaddr, size);
  if (conflict) {
    printfError("conflict when trying to map 0x%lx to 0x%lx\n", paddr, vaddr);
    return NULL;
  }

  while (remaining > 0) {
    uintptr_t currentPhys = paddr + offset;
    uintptr_t currentVirt = vaddr + offset;

    int targetLevel = 0; // default: 4 KB

    // Check if 1GB page is possible
    if ((currentPhys & (SIZE_1GB - 1)) == 0 &&
        (currentVirt & (SIZE_1GB - 1)) == 0 && remaining >= SIZE_1GB) {
      targetLevel = 2; // PDPT level = 1GB page
    }
    // Check if 2MB page is possible
    else if ((currentPhys & (SIZE_2MB - 1)) == 0 &&
             (currentVirt & (SIZE_2MB - 1)) == 0 && remaining >= SIZE_2MB) {
      targetLevel = 1; // PDT level = 2MB page
    }

    if (!mapSinglePage(currentPhys, currentVirt, targetLevel))
      return NULL;

    size_t mapped = (targetLevel == 2)   ? SIZE_1GB
                    : (targetLevel == 1) ? SIZE_2MB
                                         : SIZE_4KB;

    offset += mapped;
    remaining -= mapped;
  }

  struct VMMNode *nodePhys = slubAlloc(sizeof(struct VMMNode));
  if (!nodePhys) {
    printfError("Out of memory when trying to map  0x%lx to 0x%lx\n", paddr,
                vaddr);
    hang();
  }

  struct VMMNode *node = hhdmAdd(nodePhys);
  node->vaddr = vaddr;
  node->paddr = paddr;
  node->size = size;
  node->flags = VM_FIXED_NOREPLACE | VM_KMAP | VM_PERMANENT | VM_EXEC |
                VM_WRITE | VM_READ;
  vmmInsert(node);

  return (void *)vaddr;
}

static bool mapSinglePage(uintptr_t paddr, uintptr_t vaddr, int targetLevel) {
  // root is a virtual pointer to PML4
  uintptr_t *vtable = (uintptr_t *)root;

  // Iterate from PML4 (level 3) down to targetLevel+1 to ensure all
  // intermediate tables exist
  for (int lvl = 3; lvl > targetLevel; --lvl) {
    uintptr_t childPhys = 0;
    if (getExistingEntry(vtable, &childPhys, vaddr, lvl)) {
      // Child table exists → descend into it; childPhys holds physical addr
      vtable = (uintptr_t *)hhdmAdd((void *)childPhys);
    } else {
      // Need to create a new table. makeNewEntry returns the physical address
      // of the new child table.
      uintptr_t *childPhysPtr = makeNewEntry(vtable, paddr, vaddr, lvl);
      if (!childPhysPtr) {
        printfError("makeNewEntry failed at level %lu\n", lvl);
        return false;
      }

      // Sentinel (1) should never occur in intermediate levels
      if (childPhysPtr == (uintptr_t *)1) {
        printfError("unexpected sentinel from makeNewEntry at level %lu\n",
                    lvl);
        return false;
      }

      uintptr_t childPhysValue = (uintptr_t)childPhysPtr;
      vtable = (uintptr_t *)hhdmAdd((void *)childPhysValue);
    }
  }

  // Now vtable is the correct table at targetLevel (virtual)
  // Write the final mapping or large page
  if (targetLevel == 0) {
    int idx = getIndex(vaddr, 0);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;
  } else if (targetLevel == 1) {
    if ((paddr & (SIZE_2MB - 1)) != 0 || (vaddr & (SIZE_2MB - 1)) != 0) {
      printfError("2MB mapping requires 2MB alignment\n");
      return false;
    }
    int idx = getIndex(vaddr, 1);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW | PTE_PS;
  } else if (targetLevel == 2) {
    if ((paddr & (SIZE_1GB - 1)) != 0 || (vaddr & (SIZE_1GB - 1)) != 0) {
      printfError("1GB mapping requires 1GB alignment\n");
      return false;
    }
    int idx = getIndex(vaddr, 2);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW | PTE_PS;
  } else {
    return false;
  }

  return true;
}

// Returns child table physical address (cast as pointer), or NULL on failure.
// For targetLevel == 0 (final PTE), writes the PTE and returns (uintptr_t*)1 as
// a success sentinel.
static uintptr_t *makeNewEntry(uintptr_t *table, uintptr_t paddr,
                               uintptr_t vaddr, int targetLevel) {
  int idx = getIndex(vaddr, targetLevel);
  if (idx < 0)
    return NULL;

  uint64_t entry = table[idx];

  // Final PTE creation (level 0)
  if (targetLevel == 0) {
    table[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;
    return (uintptr_t *)1;
  }

  // Intermediate level: need a new 4KB child page table
  if (!(entry & PTE_P)) {
    uintptr_t newTablePhys = (uintptr_t)__allocpage();
    if (!newTablePhys) {
      printfError("Bro, the allocator error!\n");
      return NULL;
    }

    uintptr_t *newTableVirt = (uintptr_t *)hhdmAdd((void *)newTablePhys);
    if (!newTableVirt)
      return NULL;

    for (int i = 0; i < 512; ++i)
      newTableVirt[i] = 0;

    table[idx] = (newTablePhys & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;

    return (uintptr_t *)newTablePhys;
  }

  // Already present → return existing child table physical address
  uintptr_t childPhys = (uintptr_t)(entry & ENTRY_ADDR_MASK);
  return (uintptr_t *)childPhys;
}

// getExistingEntry: checks if an entry exists and is a valid table (not a large
// page). If valid, writes the child table physical address into outChildPhys.
static bool getExistingEntry(uintptr_t *table, uintptr_t *outChildPhys,
                             uintptr_t vaddr, int targetLevel) {
  int idx = getIndex(vaddr, targetLevel);
  if (idx < 0)
    return false;

  uint64_t entry = table[idx];

  if (!(entry & PTE_P))
    return false;

  if (entry & PTE_PS) {
    return false;
  }

  uintptr_t childPhys = (uintptr_t)(entry & ENTRY_ADDR_MASK);
  if (outChildPhys)
    *outChildPhys = childPhys;
  return true;
}
