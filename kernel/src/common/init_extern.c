#include <kernel_info.h>
#include <macros.h>
#include <mm/memmap.h>
#include <mm/vmm.h>
#include <stack.h>
#include <stdint.h>

struct MemoryMap memmap = {0};
uintptr_t stackAddress = 0;
uintptr_t hhdmOffset = 0;
uintptr_t kernelPhysAddr = 0;
uintptr_t kernelVirtAddr = 0;