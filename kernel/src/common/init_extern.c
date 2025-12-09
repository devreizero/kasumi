#include "cpu/topology.h"
#include <kernel_info.h>
#include <macros.h>
#include <mm/memmap.h>
#include <mm/vmm.h>
#include <stack.h>
#include <stdint.h>

struct VMMTree vmmTree = {0};
struct VMMNode vmmNilNode = {0};
struct MemoryMap memmap = {0};
uintptr_t stackAddress = 0;
uintptr_t hhdmOffset = 0;
uintptr_t kernelPhysAddr = 0;
uintptr_t kernelVirtAddr = 0;
uint32_t cpuID0 = 0;
uint32_t cpuCount = 1;
uint32_t *cpuIDs = &cpuID0;