#pragma once
#ifndef VMM_H
#define VMM_H

#include <macros.h>
#include <stdint.h>

extern __initdata uintptr_t hhdmOffset;

__init void *hhdmAdd(void *paddr);
__init void *hhdmRemove(void *vaddr);
__init uintptr_t hhdmAddAddr(uintptr_t paddr);
__init uintptr_t hhdmRemoveAddr(uintptr_t vaddr);

__init void vmmInit();
uint16_t getPageCountFromRange(uintptr_t start, uintptr_t end);

#endif