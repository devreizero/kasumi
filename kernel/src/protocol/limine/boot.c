#include <arch-hook.h>
#include <basic-io.h>
#include <limine/deploy.h>
#include <limine/limine.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <serial.h>
#include <stack.h>
#include <stdbool.h>

LIMINE_BASE_REF();
LIMINE_GET_HHDM();

void kboot() {
  stackAddress = getStackAddress();

  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hang();
  }

  hhdmOffset = LIMINE_REQ(hhdm).response->offset;

  archEarlyInit();
  serialPuts("Hello, World! From Kernel!\r\n");

  memmapAbstract();
  serialPuts("Something, something\r\n");

  pmmInit();
  
  serialPuts("PMM end, something\r\n");

  hang();
}