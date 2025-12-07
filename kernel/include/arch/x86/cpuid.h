#pragma once
#ifndef X86_CPUID_H
#define X86_CPUID_H

#include <stdint.h>

static inline void cpuid(uint32_t eaxIn, uint32_t ecxIn,
                         uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a" (*eax),
          "=b" (*ebx),
          "=c" (*ecx),
          "=d" (*edx)
        : "a" (eaxIn),
          "c" (ecxIn)
    );
}

#endif