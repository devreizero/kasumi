#include <macros.h>
#include <mm/vmm.h>
#include <stdint.h>

__init void *hhdmAdd(void *paddr) {
    return (void *) ((uintptr_t) paddr + hhdmOffset);
}

__init void *hhdmRemove(void *vaddr) {
    return (void *) ((uintptr_t) vaddr - hhdmOffset);
}

__init uintptr_t hhdmAddAddr(uintptr_t paddr) {
    return paddr + hhdmOffset;
}

__init uintptr_t hhdmRemoveAddr(uintptr_t vaddr) {
    return vaddr - hhdmOffset;
}