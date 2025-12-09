#include <basic_io.h>
#include <kernel_info.h>
#include <macros.h>
#include <mm/hhdm.h>
#include <mm/kmap.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/zone.h>
#include <panic.h>
#include <printf.h>
#include <stack.h>
#include <stddef.h>
#include <stdint.h>
#include <stdmem.h>
#include <x86/cpuid.h>
#include <x86/page_table.h>

extern uint8_t __kernelStart;
extern uint8_t __kernelEnd;

extern uint8_t __textInitSectionStart;
extern uint8_t __textInitSectionEnd;

extern uint8_t __textSectionStart;
extern uint8_t __textSectionEnd;

extern uint8_t __rodataSectionStart;
extern uint8_t __rodataSectionEnd;

extern uint8_t __dataInitSectionStart;
extern uint8_t __dataInitSectionEnd;

extern uint8_t __dataSectionStart;
extern uint8_t __dataSectionEnd;

size_t pagePhysicalMask;
size_t pagePhysicalBits;
size_t pageVirtualBits;

#ifdef ARCH_64
uint64_t *root;
#else
#ifndef X86_NO_PAE
uint32_t root[1024] __alignment(4096);
#else
uint64_t root[4] __alignment(32);
#endif
#endif

__init static void mapMemories();
__init static void mapMemoryMap(struct MemoryEntries *entries);
__init static void mapEntry(struct MemoryMapEntry *entry);

void vmmInit() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
  pageVirtualBits = (eax >> 8) & 0xFF;
  pagePhysicalBits = eax & 0xFF;
  pagePhysicalMask = ((1ULL << pagePhysicalBits) - 1) & ~0xFFFULL;

  root = pageAlloc(ZONE_NORMAL, 1);
  if (!root) {
    panic("Failed to alloc PML4: 0x%lx\n", root);
    return;
  }

  root = hhdmAdd(root);
  memset(root, 0, 4096);

#if !defined(ARCH_64) && !defined(X86_NO_PAE)
  uintpr_t root2 = __aligndown((uintptr_t)root, 32);
  root = (uint64_t *)root2;
#endif

  vmmInitRBTree();
  mapMemories();

  uintptr_t physAddr = hhdmRemoveAddr((uintptr_t)root);
  __asm__ volatile("mov %0, %%cr3" ::"r"(physAddr) : "memory");

  printfInfo("Paging taken over\n");
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Red-Black Tree
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void rbRotateLeft(struct VMMNode *node);
static void rbRotateRight(struct VMMNode *node);
static void rbInsertFixup(struct VMMNode *node);
static void rbDeleteFixup(struct VMMNode *node);
static struct VMMNode *rbTreeMinimum(struct VMMNode *node);

__init void vmmInitRBTree() {
  vmmNilNode.color = VMM_RB_NIL;
  vmmNilNode.flags = 0;
  vmmNilNode.paddr = 0;
  vmmNilNode.vaddr = 0;
  vmmNilNode.size = 0;
  vmmNilNode.left = &vmmNilNode;
  vmmNilNode.right = &vmmNilNode;
  vmmNilNode.parent = &vmmNilNode;
  vmmTree.root = &vmmNilNode;
}

struct VMMNode *vmmFindNodeContaining(uintptr_t vaddr) {
  struct VMMNode *node = vmmTree.root;

  while (node != &vmmNilNode) {
    uintptr_t start = node->vaddr;
    uintptr_t end = start + node->size;

    if (vaddr >= start && vaddr < end)
      return node;

    if (vaddr < start)
      node = node->left;
    else
      node = node->right;
  }

  return NULL;
}

struct VMMNode *vmmFindNodeOverlapping(uintptr_t start, size_t size) {
  uintptr_t end = start + size;
  struct VMMNode *node = vmmTree.root;

  while (node != &vmmNilNode) {
    uintptr_t nStart = node->vaddr;
    uintptr_t nEnd = nStart + node->size;

    // Check overlap
    if (start < nEnd && nStart < end)
      return node;

    if (end <= nStart) {
      node = node->left;
    } else {
      node = node->right;
    }
  }

  return NULL;
}

void vmmInsert(struct VMMNode *node) {
  struct VMMNode *y = &vmmNilNode;
  struct VMMNode *x = vmmTree.root;

  while (x != &vmmNilNode) {
    y = x;
    if (node->vaddr < x->vaddr)
      x = x->left;
    else
      x = x->right;
  }

  node->parent = y;

  if (y == &vmmNilNode)
    vmmTree.root = node;
  else if (node->vaddr < y->vaddr)
    y->left = node;
  else
    y->right = node;

  node->left = &vmmNilNode;
  node->right = &vmmNilNode;
  node->color = VMM_RB_RED;

  rbInsertFixup(node);
}

void vmmDelete(struct VMMNode *z) {
  struct VMMNode *y = z;
  struct VMMNode *x;
  enum VMMRBColor y_original_color = y->color;

  if (z->left == &vmmNilNode) {
    x = z->right;
    if (z->parent == &vmmNilNode)
      vmmTree.root = z->right;
    else if (z == z->parent->left)
      z->parent->left = z->right;
    else
      z->parent->right = z->right;

    z->right->parent = z->parent;

  } else if (z->right == &vmmNilNode) {
    x = z->left;
    if (z->parent == &vmmNilNode)
      vmmTree.root = z->left;
    else if (z == z->parent->left)
      z->parent->left = z->left;
    else
      z->parent->right = z->left;

    z->left->parent = z->parent;

  } else {
    y = rbTreeMinimum(z->right);
    y_original_color = y->color;
    x = y->right;

    if (y->parent == z) {
      x->parent = y;
    } else {
      if (y == y->parent->left)
        y->parent->left = y->right;
      else
        y->parent->right = y->right;

      y->right->parent = y->parent;

      y->right = z->right;
      y->right->parent = y;
    }

    if (z->parent == &vmmNilNode)
      vmmTree.root = y;
    else if (z == z->parent->left)
      z->parent->left = y;
    else
      z->parent->right = y;

    y->parent = z->parent;
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
  }

  if (y_original_color == VMM_RB_BLACK)
    rbDeleteFixup(x);
}

static void rbInsertFixup(struct VMMNode *z) {
  while (z->parent->color == VMM_RB_RED) {
    if (z->parent == z->parent->parent->left) {
      struct VMMNode *y = z->parent->parent->right;

      if (y->color == VMM_RB_RED) {
        z->parent->color = VMM_RB_BLACK;
        y->color = VMM_RB_BLACK;
        z->parent->parent->color = VMM_RB_RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rbRotateLeft(z);
        }
        z->parent->color = VMM_RB_BLACK;
        z->parent->parent->color = VMM_RB_RED;
        rbRotateRight(z->parent->parent);
      }

    } else {
      struct VMMNode *y = z->parent->parent->left;

      if (y->color == VMM_RB_RED) {
        z->parent->color = VMM_RB_BLACK;
        y->color = VMM_RB_BLACK;
        z->parent->parent->color = VMM_RB_RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rbRotateRight(z);
        }
        z->parent->color = VMM_RB_BLACK;
        z->parent->parent->color = VMM_RB_RED;
        rbRotateLeft(z->parent->parent);
      }
    }
  }

  vmmTree.root->color = VMM_RB_BLACK;
}

static void rbDeleteFixup(struct VMMNode *x) {
  while (x != vmmTree.root && x->color == VMM_RB_BLACK) {
    if (x == x->parent->left) {
      struct VMMNode *w = x->parent->right;

      if (w->color == VMM_RB_RED) {
        w->color = VMM_RB_BLACK;
        x->parent->color = VMM_RB_RED;
        rbRotateLeft(x->parent);
        w = x->parent->right;
      }

      if (w->left->color == VMM_RB_BLACK && w->right->color == VMM_RB_BLACK) {
        w->color = VMM_RB_RED;
        x = x->parent;
      } else {
        if (w->right->color == VMM_RB_BLACK) {
          w->left->color = VMM_RB_BLACK;
          w->color = VMM_RB_RED;
          rbRotateRight(w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = VMM_RB_BLACK;
        w->right->color = VMM_RB_BLACK;
        rbRotateLeft(x->parent);
        x = vmmTree.root;
      }

    } else {
      struct VMMNode *w = x->parent->left;

      if (w->color == VMM_RB_RED) {
        w->color = VMM_RB_BLACK;
        x->parent->color = VMM_RB_RED;
        rbRotateRight(x->parent);
        w = x->parent->left;
      }

      if (w->right->color == VMM_RB_BLACK && w->left->color == VMM_RB_BLACK) {
        w->color = VMM_RB_RED;
        x = x->parent;
      } else {
        if (w->left->color == VMM_RB_BLACK) {
          w->right->color = VMM_RB_BLACK;
          w->color = VMM_RB_RED;
          rbRotateLeft(w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = VMM_RB_BLACK;
        w->left->color = VMM_RB_BLACK;
        rbRotateRight(x->parent);
        x = vmmTree.root;
      }
    }
  }
  x->color = VMM_RB_BLACK;
}

static void rbRotateLeft(struct VMMNode *x) {
  struct VMMNode *y = x->right;
  x->right = y->left;

  if (y->left != &vmmNilNode)
    y->left->parent = x;

  y->parent = x->parent;

  if (x->parent == &vmmNilNode)
    vmmTree.root = y;
  else if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;

  y->left = x;
  x->parent = y;
}

static void rbRotateRight(struct VMMNode *y) {
  struct VMMNode *x = y->left;
  y->left = x->right;

  if (x->right != &vmmNilNode)
    x->right->parent = y;

  x->parent = y->parent;

  if (y->parent == &vmmNilNode)
    vmmTree.root = x;
  else if (y == y->parent->right)
    y->parent->right = x;
  else
    y->parent->left = x;

  x->right = y;
  y->parent = x;
}

static struct VMMNode *rbTreeMinimum(struct VMMNode *x) {
  while (x->left != &vmmNilNode)
    x = x->left;
  return x;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory Mapping
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

__init static void mapMemories() {
  // MEMMAP
  mapMemoryMap(memmap.usable);
  mapMemoryMap(memmap.bootloaderReclaimable);
  mapMemoryMap(memmap.acpiReclaimable);
  mapMemoryMap(memmap.acpiNvs);
  mapMemoryMap(memmap.acpiTable);
  mapMemoryMap(memmap.reserved);
  mapEntry(memmap.kernel);
  mapEntry(memmap.allocator);
  mapEntry(memmap.framebuffer);

  // ALLOCATOR
  pmmMap();

  // KERNEL
  uintptr_t kernelStart = __aligndown((uintptr_t)&__kernelStart, ALIGN_4KB);
  uintptr_t kernelEnd = __alignup((uintptr_t)&__kernelEnd, ALIGN_4KB);
  size_t kernelSize = kernelEnd - kernelStart;
  kmap(kernelPhysAddr, kernelVirtAddr, kernelSize);

  // STACK
  uintptr_t stackAddr = __aligndown(stackAddress, ALIGN_4KB);
  kmap(hhdmRemoveAddr(stackAddr), stackAddr, SIZE_4KB);
}

__init static void mapMemoryMap(struct MemoryEntries *entries) {
  uintptr_t alignedStart;
  for (size_t i = 0; i < entries->count; i++) {
    alignedStart = __aligndown(entries->entries[i].base, ALIGN_4KB);
    kmap(alignedStart, hhdmAddAddr(alignedStart),
         __alignup(entries->entries[i].length, ALIGN_4KB));
  }
}

__init static void mapEntry(struct MemoryMapEntry *entry) {
  uintptr_t alignedStart = __aligndown(entry->base, ALIGN_4KB);
  kmap(alignedStart, hhdmAddAddr(alignedStart),
       __alignup(entry->length, ALIGN_4KB));
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t getPageCountFromRange(uintptr_t start, uintptr_t end) {
  return (end - start + PAGE_SIZE - 1) / PAGE_SIZE;
}