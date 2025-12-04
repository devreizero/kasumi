#pragma once
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#ifdef PAGE_SIZE_2MB
#define PAGE_SIZE 2097152
#define PAGE_SHIFT 21
#else
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#endif

enum ZoneType;

void pmmInit();
void pmmDumpStats(void);
void *pageAlloc(enum ZoneType type, size_t pageCount); 
void *pageAllocAligned(enum ZoneType type, size_t pageCount, size_t alignment);
void pageFree(void *paddr);

#endif