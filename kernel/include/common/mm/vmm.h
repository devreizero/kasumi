#pragma once
#ifndef VMM_H
#define VMM_H

#include <macros.h>
#include <mm/mmap.h>
#include <stdint.h>

extern __initdata uintptr_t hhdmOffset;

__init void *hhdmAdd(void *paddr);
__init void *hhdmRemove(void *vaddr);

__init void initPageTable();

#endif