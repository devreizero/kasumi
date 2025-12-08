#include <basic-io.h>
#include <mm/memmap.h>
#include <mm/zone.h>
#include <stack.h>
#include <format_string.h>
#include <kernel_info.h>
#include <printf.h>
#include <macros.h>
#include <mm/kmap.h>
#include <mm/pmm.h>
#include <mm/hhdm.h>
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

uint16_t getPageCountFromRange(uintptr_t start, uintptr_t end) {
    return (end - start + PAGE_SIZE - 1) / PAGE_SIZE;
}

static void mapMemoryMap(struct MemoryEntries *entries) {
    uintptr_t alignedStart;
    for (size_t i = 0; i < entries->count; i++) {
        alignedStart = __aligndown(entries->entries[i].base, ALIGN_4KB);
        kmap(alignedStart, hhdmAddAddr(alignedStart), __alignup(entries->entries[i].length, ALIGN_4KB));
    }
}

static void mapEntry(struct MemoryMapEntry *entry) {
    uintptr_t alignedStart = __aligndown(entry->base, ALIGN_4KB);
    kmap(alignedStart, hhdmAddAddr(alignedStart), __alignup(entry->length, ALIGN_4KB));
}

void vmmInit() {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
    pageVirtualBits  = (eax >> 8) & 0xFF;
    pagePhysicalBits = eax        & 0xFF;
    pagePhysicalMask = ((1ULL << pagePhysicalBits) - 1) & ~0xFFFULL;

    root = pageAlloc(ZONE_NORMAL, 1);
    if (!root) {
        printfError("Failed to alloc PML4: 0x%lx\n", root);
        hang();
        return;
    }

    root = hhdmAdd(root);

    memset(root, 0, 4096);

    ////// MAPPING

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
    uintptr_t kernelStart = __aligndown((uintptr_t) &__kernelStart, ALIGN_4KB);
    uintptr_t kernelEnd   = __alignup((uintptr_t) &__kernelEnd, ALIGN_4KB);
    size_t    kernelSize  = kernelEnd - kernelStart;
    kmap(kernelPhysAddr, kernelVirtAddr, kernelSize);

    // STACK
    uintptr_t stackAddr   = __aligndown(stackAddress, ALIGN_4KB);
    kmap(hhdmRemoveAddr(stackAddr), stackAddr, SIZE_4KB);

    uintptr_t physAddr = hhdmRemoveAddr((uintptr_t) root);
    __asm__ volatile ("mov %0, %%cr3" :: "r"(physAddr) : "memory");

    printfInfo("Paging taken over\n");
}