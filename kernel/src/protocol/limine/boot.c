#include <arch-hook.h>
#include <basic_io.h>
#include <kernel_info.h>
#include <limine/deploy.h>
#include <limine/limine.h>
#include <macros.h>
#include <mm/hhdm.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <mm/slub.h>
#include <mm/vmm.h>
#include <printf.h>
#include <serial.h>
#include <stack.h>
#include <stdbool.h>
#include <stdmem.h>

LIMINE_BASE_REF();
LIMINE_GET_HHDM();
LIMINE_GET_EXECUTABLE_ADDR();

extern __noreturn void kmain();

__noreturn void kboot() {
  stackAddress = getStackAddress();

  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hang();
  }

  hhdmOffset = LIMINE_REQ(hhdm).response->offset;
  kernelPhysAddr = LIMINE_REQ(executableAddress).response->physical_base;
  kernelVirtAddr = LIMINE_REQ(executableAddress).response->virtual_base;

  archEarlyInit();
  printfOk("Hello, World! From Kernel!\n");

  memmapAbstract();
  memmapDump();

  pmmInit();
  slubInit();
  vmmInit();

  printfOk(
      "World End Destruction, Alternating Universe, Moving to KMAIN()!!\n");

  kmain();
}