#include <printf.h>
#include <arch-hook.h>
#include <basic-io.h>
#include <kernel_info.h>
#include <limine/deploy.h>
#include <limine/limine.h>
#include <mm/memmap.h>
#include <mm/hhdm.h>
#include <mm/pmm.h>
#include <mm/slub.h>
#include <mm/vmm.h>
#include <serial.h>
#include <stack.h>
#include <stdbool.h>
#include <stdmem.h>

LIMINE_BASE_REF();
LIMINE_GET_HHDM();
LIMINE_GET_EXECUTABLE_ADDR();

void kboot() {
  stackAddress = getStackAddress();

  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hang();
  }

  hhdmOffset        = LIMINE_REQ(hhdm).response->offset;
  kernelPhysAddr    = LIMINE_REQ(executableAddress).response->physical_base;
  kernelVirtAddr    = LIMINE_REQ(executableAddress).response->virtual_base;

  archEarlyInit();
  printfOk("Hello, World! From Kernel!\n");

  memmapAbstract();

  memmapDump();

  pmmInit();

  vmmInit();

  slubInit();
  // pmmDumpStats(true);

  void *paddr = slubAlloc(204);
  printf("From SLUB=0x%lx\n", paddr);

  void *vaddr = hhdmAdd(paddr);
  memset(vaddr, 'A', 10);
  ((char*)vaddr)[10] = 0;

  printf("Value=%s\n", vaddr);

  slubDumpStats();
  
  serialPuts("World End Destruction!!\r\n");

  hang();
}