#include "mm/mmap.h"
#include <mm/vmm.h>
#include <stdint.h>

extern uintptr_t __kernelStart;
extern uintptr_t __kernelEnd;

extern uintptr_t __codeInitSectionEnd;
extern uintptr_t __codeSectionEnd;

extern uintptr_t __textSectionEnd;
extern uintptr_t __rodataSectionEnd;

extern uintptr_t __dataInitSectionEnd;
extern uintptr_t __dataSectionEnd;

void initPageTable() {
    // mmap((void *) __kernelStart, __textSectionEnd - __kernelStart, PROT_EXEC | PROT_READ, MAP_ANONYMOUS, -1, 0);
}