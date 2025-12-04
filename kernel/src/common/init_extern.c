#include <macros.h>
#include <mm/memmap.h>
#include <mm/vmm.h>
#include <stack.h>
#include <stdint.h>

struct MemoryMap memmap = {0};
uintptr_t stackAddress = 0;
__initdata uintptr_t hhdmOffset = 0;