#pragma once
#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>
#include <stddef.h>

enum MemoryProtection {
    PROT_NONE   = 0,
    PROT_READ   = 1 << 0,
    PROT_WRITE  = 1 << 1,
    PROT_EXEC   = 1 << 2
};

enum MemoryMappingFlags {
    MAP_SHARED          = 1 << 0,
    MAP_PRIVATE         = 1 << 1,
    MAP_ANONYMOUS       = 1 << 2,
    MAP_ANON            = 1 << 2,
    MAP_FIXED           = 1 << 3,
    MAP_FIXED_NOREPLACE = 1 << 4,
    MAP_NORESERVE       = 1 << 5,
    MAP_POPULATE        = 1 << 6,
    MAP_STACK           = 1 << 7,
    MAP_GROWSDOWN       = 1 << 8,
    MAP_HUGE_2MB        = 1 << 10,
    MAP_HUGE_1GB        = 1 << 11,
    MAP_32BIT           = 1 << 12,
    MAP_SYNC            = 1 << 13
};

void *mmap(void *addr, size_t length,  int prot, int flags, int fd, uintptr_t offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t length, int prot);

#endif