#pragma once
#ifndef VMM_H
#define VMM_H

#include <macros.h>
#include <stdint.h>

__init void vmmInit();
uint16_t getPageCountFromRange(uintptr_t start, uintptr_t end);

#endif