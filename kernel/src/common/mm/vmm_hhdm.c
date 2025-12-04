#include <macros.h>
#include <mm/vmm.h>
#include <stdint.h>

__init void *hhdmAdd(void *paddr) {
    return (void *) ((uintptr_t) paddr + hhdmOffset);
}

__init void *hhdmRemove(void *vaddr) {
    return (void *) ((uintptr_t) vaddr - hhdmOffset);
}