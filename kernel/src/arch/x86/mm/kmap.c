#include <assert.h>
#include <macros.h>
#include <mm/kmap.h>
#include <mm/pmm.h>
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

static bool mapSinglePage(uintptr_t paddr, uintptr_t vaddr,
                                 int targetLevel);
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

  // printfDebug("@@ Mapping paddr 0x%lx -> vaddr 0x%lx until 0x%lx\n",
  //             paddr, vaddr, vaddr + size);

  while (remaining > 0) {
      uintptr_t curr_p = paddr + offset;
      uintptr_t curr_v = vaddr + offset;

      int targetLevel = 0; // default: 4 KB

      // 1GB page possible?
      if ((curr_p & (SIZE_1GB - 1)) == 0 &&
          (curr_v & (SIZE_1GB - 1)) == 0 &&
          remaining >= SIZE_1GB) 
      {
          targetLevel = 2; // PDPT level = 1GB page
      }
      // 2MB page possible?
      else if ((curr_p & (SIZE_2MB - 1)) == 0 &&
               (curr_v & (SIZE_2MB - 1)) == 0 &&
               remaining >= SIZE_2MB)
      {
          targetLevel = 1; // PDT level = 2MB page
      }

      if (!mapSinglePage(curr_p, curr_v, targetLevel))
          return NULL;

      size_t mapped = (targetLevel == 2) ? SIZE_1GB :
                      (targetLevel == 1) ? SIZE_2MB :
                                           SIZE_4KB;

      offset += mapped;
      remaining -= mapped;
  }

  return (void *)vaddr;
}

static bool mapSinglePage(uintptr_t paddr, uintptr_t vaddr,
                                 int targetLevel) {
  // root is virtual pointer to PML4
  uintptr_t *vtable = (uintptr_t *)root;

  // iterasi dari PML4(level 3) turun sampai level targetLevel+1 untuk
  // memastikan semua intermediate tables ada
  for (int lvl = 3; lvl > targetLevel; --lvl) {
    uintptr_t child_phys = 0;
    if (getExistingEntry(vtable, &child_phys, vaddr, lvl)) {
      // ada child table → naik: child_phys adalah physical address table
      vtable = (uintptr_t *)hhdmAdd((void *)child_phys);
    } else {
      // perlu buat table baru : makeNewEntry mengembalikan physical address
      // child table (cast pointer)
      uintptr_t *child_phys_ptr = makeNewEntry(vtable, paddr, vaddr, lvl);
      if (!child_phys_ptr) {
        printfError("makeNewEntry failed at level %lu\n", lvl);
        return false;
      }

      // jika makeNewEntry mengembalikan sentinel (1) itu artinya ia menulis PTE
      // langsung — tapi di loop ini kita tidak pernah memanggil targetLevel==0,
      // jadi seharusnya tidak terjadi
      if (child_phys_ptr == (uintptr_t *)1) {
        // shouldn't happen in intermediate levels
        printfError("unexpected sentinel from makeNewEntry at level %lu\n", lvl);
        return false;
      }

      // convert phys->virt
      uintptr_t child_phys_val = (uintptr_t)child_phys_ptr;
      vtable = (uintptr_t *)hhdmAdd((void *)child_phys_val);
    }
  }

  // sekarang vtable adalah table di level targetLevel (virtual)
  // buat entry final (atau big page) di sini
  if (targetLevel == 0) {
    // final 4K PTE
    int idx = getIndex(vaddr, 0);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;
  } else if (targetLevel == 1) {
    // PD -> create 2MB PDE (ensure alignment & check PS choice)
    if ((paddr & (SIZE_2MB - 1)) != 0 || (vaddr & (SIZE_2MB - 1)) != 0) {
      // fallback to 4K mapping or return error; here: return false
      printfError("2MB mapping requires 2MB alignment\n");
      return false;
    }
    int idx = getIndex(vaddr, 1);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW | PTE_PS;
  } else if (targetLevel == 2) {
    // PDPT -> 1GB page
    if ((paddr & (SIZE_1GB - 1)) != 0 || (vaddr & (SIZE_1GB - 1)) != 0) {
      printfError("1GB mapping requires 1GB alignment\n");
      return false;
    }
    int idx = getIndex(vaddr, 2);
    if (idx < 0)
      return false;
    vtable[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW | PTE_PS;
  } else {
    // shouldn't map targetLevel > 2 as a final mapping
    return false;
  }

  // printfInfo("Mapping %s -- vaddr 0x%lx -> paddr 0x%lx\n",
  //           SIZE_STR[targetLevel - 1], vaddr, paddr);

  return true;
}

// returns child physical as pointer-cast, or NULL on failure.
// For targetLevel == 0 (final PTE) it writes the PTE and returns (uintptr_t*)1
// to indicate success.
static uintptr_t *makeNewEntry(uintptr_t *table, uintptr_t paddr,
                                      uintptr_t vaddr, int targetLevel) {
  int idx = getIndex(vaddr, targetLevel);
  if (idx < 0)
    return NULL;

  uint64_t entry = table[idx];

  // Final PTE creation (level 0)
  if (targetLevel == 0) {
    // write the page table entry: address + flags
    table[idx] = (paddr & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;
    // Optionally set PTE_US or PTE_NX as needed
    return (uintptr_t *)1; // non-NULL sentinel (no child table)
  }

  // Intermediate level: need a child page-table (4KB)
  if (!(entry & PTE_P)) {
    // allocate a new page (physical)
    uintptr_t new_table_phys = (uintptr_t)__allocpage();
    if (!new_table_phys) {
      printfError("Bro, the allocator error!\n");
      return NULL;
    }

    // Convert to virtual to zero it
    uintptr_t *new_table_virt = (uintptr_t *)hhdmAdd((void *)new_table_phys);
    if (!new_table_virt)
      return NULL;

    // zero the page
    for (int i = 0; i < 512; ++i)
      new_table_virt[i] = 0;

    // set parent entry to point to new table (physical) with flags
    table[idx] = (new_table_phys & ENTRY_ADDR_MASK) | PTE_P | PTE_RW;

    // return physical address as pointer (so caller can hhdmAdd it)
    return (uintptr_t *)new_table_phys;
  }

  // Already present -> return the child table physical address (clear flags)
  uintptr_t child_phys = (uintptr_t)(entry & ENTRY_ADDR_MASK);
  return (uintptr_t *)child_phys;
}

// getExistingEntry: checks if entry present and is a table (not a large-page).
// If present and is a table, writes child physical address into *out_child_phys
// and returns true. out_child_phys must be pointer to uintptr_t (call with
// &child_phys).
static bool getExistingEntry(uintptr_t *table, uintptr_t *out_child_phys,
                                    uintptr_t vaddr, int targetLevel) {
  int idx = getIndex(vaddr, targetLevel);
  if (idx < 0)
    return false;

  uint64_t entry = table[idx];

  if (!(entry & PTE_P))
    return false; // not present

  // If this entry is a huge page (PS set) then we cannot descend
  if (entry & PTE_PS) {
    return false; // present but it's a large page, not a table
  }

  // extract child physical address
  uintptr_t child_phys = (uintptr_t)(entry & ENTRY_ADDR_MASK);
  if (out_child_phys)
    *out_child_phys = child_phys;
  return true;
}