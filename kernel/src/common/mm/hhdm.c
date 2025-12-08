#include <macros.h>
#include <mm/hhdm.h>
#include <stdint.h>

void *hhdmAdd(void *paddr) {
    return (void *) ((uintptr_t) paddr + hhdmOffset);
}

void *hhdmRemove(void *vaddr) {
    return (void *) ((uintptr_t) vaddr - hhdmOffset);
}

uintptr_t hhdmAddAddr(uintptr_t paddr) {
    return paddr + hhdmOffset;
}

uintptr_t hhdmRemoveAddr(uintptr_t vaddr) {
    return vaddr - hhdmOffset;
}