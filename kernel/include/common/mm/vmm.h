#pragma once
#ifndef VMM_H
#define VMM_H

#include <macros.h>
#include <stdint.h>
#include <stddef.h>

/**
 * VMM Mapping flags
 */
enum VMMMapFlags {
    VM_READ             = 1 << 0,
    VM_WRITE            = 1 << 1,
    VM_EXEC             = 1 << 2,
    VM_KMAP             = 1 << 3, // kernel mapped
    VM_PERMANENT        = 1 << 4,
    VM_FIXED            = 1 << 5,
    VM_FIXED_NOREPLACE  = 1 << 6
};

/**
 * Classic Red-Black Tree Colors
 */
enum VMMRBColor {
    VMM_RB_RED,
    VMM_RB_BLACK,
    VMM_RB_NIL
};

struct VMMNode {
    uintptr_t vaddr;    // virtual address base
    uintptr_t paddr;    // physical address base
    size_t size;        // length in bytes
    uint32_t flags;     // read/write/exec/kernel/etc

    enum VMMRBColor color;
    struct VMMNode *parent;
    struct VMMNode *left;
    struct VMMNode *right;
};

struct VMMTree {
    struct VMMNode *root;
};

extern struct VMMTree vmmTree;
extern struct VMMNode vmmNilNode;

__init void vmmInitRBTree();
struct VMMNode *vmmFindNodeContaining(uintptr_t vaddr);
struct VMMNode *vmmFindNodeOverlapping(uintptr_t start, size_t size);
void vmmInsert(struct VMMNode *node);
void vmmDelete(struct VMMNode *node);

__init void vmmInit();
uint16_t getPageCountFromRange(uintptr_t start, uintptr_t end);

#endif