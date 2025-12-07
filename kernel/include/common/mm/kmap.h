#pragma once
#ifndef KMAP_H
#define KMAP_H

#include <macros.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Kernel mapping flags.
 */
enum KMapFlags {
    KMAP_NONE           = 0,
    KMAP_READ           = 1 << 0,
    KMAP_WRITE          = 1 << 1,
    KMAP_EXEC           = 1 << 2,
    KMAP_CACHE          = 1 << 3,  // Allow CPU caching
    KMAP_NOCACHE        = 1 << 4,  // Disable caching
    KMAP_DEVICE         = 1 << 5,  // Device memory
    KMAP_WC             = 1 << 6,  // Write-combining
    KMAP_FIXED          = 1 << 7,  // Virtual addr is not a hint
    KMAP_NOREPLACE      = 1 << 8   // Don't replace existing vaddr
};

/**
 * kmalloc-like allocation for kernel mappings.
 * Returns a kernel virtual address that maps the given physical address.
 *
 * @param paddr: Physical address to map
 * @param vaddr: Virtual address to map
 * @param size: Size in bytes
 * @return virtual address in kernel space
 */
void *kmap(uintptr_t paddr, uintptr_t vaddr, size_t size);

/**
 * kmalloc-like allocation for kernel mappings.
 * Returns a kernel virtual address that maps the given physical address.
 *
 * @param paddr: Physical address to map
 * @param vaddr: Virtual address to map
 * @param size: Size in bytes
 * @param flags: Combination of KMapFlags
 * @return virtual address in kernel space
 */

/**
 * Unmap a previously mapped kernel virtual address.
 *
 * @param vptr: Kernel virtual address returned by kmap
 * @param size: Size in bytes
 * @return 0 on success, negative on error
 */
int kunmap(void *vptr, size_t size);

/**
 * Temporary mapping of a single page.
 * Often used in low-level kernel code for accessing specific physical memory.
 *
 * @param paddr: Physical address of page
 * @return kernel virtual address of mapped page
 */
void *kmapAtomic(uintptr_t paddr) __unimplemented("kmapAtomic");

/**
 * Undo temporary mapping created by kmap_atomic
 *
 * @param vptr: Kernel virtual address returned by kmapAtomic
 */
void kunmapAtomic(void *vptr) __unimplemented("kunmapAtomic");

#endif