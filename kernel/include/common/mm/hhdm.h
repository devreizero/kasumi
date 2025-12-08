#pragma once
#ifndef HHDM_H
#define HHDM_H

#include <stdint.h>

extern uintptr_t hhdmOffset;

void *hhdmAdd(void *paddr);
void *hhdmRemove(void *vaddr);
uintptr_t hhdmAddAddr(uintptr_t paddr);
uintptr_t hhdmRemoveAddr(uintptr_t vaddr);

#endif