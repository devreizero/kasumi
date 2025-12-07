#pragma once
#ifndef X86_PAGE_TABLE_H
#define X86_PAGE_TABLE_H

enum PTE64 {
    PTE64_PRESENT       = 1ULL << 0,
    PTE64_RW            = 1ULL << 1,
    PTE64_US            = 1ULL << 2,
    PTE64_PWT           = 1ULL << 3,
    PTE64_PCD           = 1ULL << 4,
    PTE64_ACCESSED      = 1ULL << 5,
    PTE64_DIRTY         = 1ULL << 6,
    PTE64_PAT           = 1ULL << 7,
    PTE64_GLOBAL        = 1ULL << 8,
    PTE64_AVL           = 0b111ULL << 9,
    PTE64_AVL2          = 0b11111ULL << 52,
    PTE64_PK            = 0b1111ULL << 58,  
    PTE64_NX            = 1ULL << 63
};

enum PD64 {
    PD64_PRESENT        = 1ULL << 0,
    PD64_RW             = 1ULL << 1,
    PD64_US             = 1ULL << 2,
    PD64_PWT            = 1ULL << 3,
    PD64_PCD            = 1ULL << 4,
    PD64_ACCESSED       = 1ULL << 5,
    PD64_DIRTY          = 1ULL << 6,
    PD64_PS             = 1ULL << 7,
    PD64_GLOBAL         = 1ULL << 8,
    PD64_AVL            = 0b111ULL << 9,
    PD64_PAT            = 1ULL << 12,
    P64_AVL2            = 0b11111ULL << 52,
    P64_PK              = 0b1111ULL << 58,  
    P64_NX              = 1ULL << 63
};

enum PTE32 {
    PTE32_PRESENT       = 1 << 0,
    PTE32_RW            = 1 << 1,
    PTE32_US            = 1 << 2,
    PTE32_PWT           = 1 << 3,
    PTE32_PCD           = 1 << 4,
    PTE32_ACCESSED      = 1 << 5,
    PTE32_DIRTY         = 1 << 6,
    PTE32_PAT           = 1 << 7,
    PTE32_GLOBAL        = 1 << 8,
    PTE32_AVL           = 0b111 << 9,
    PTE32_ADDR_MASK     = 0xFFFFF000
};

enum PD32 {
    PD32_PRESENT        = 1 << 0,
    PD32_RW             = 1 << 1,
    PD32_US             = 1 << 2,
    PD32_PWT            = 1 << 3,
    PD32_PCD            = 1 << 4,
    PD32_ACCESSED       = 1 << 5,
    PD32_DIRTY          = 1 << 6,
    PD32_PS             = 1 << 7,
    PD32_GLOBAL         = 1 << 8,
    PD32_AVL            = 0b111 << 9,
    PD32_ADDR_MASK      = 0xFFFFF000 
};

#endif