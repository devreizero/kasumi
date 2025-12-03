#include <arch-hook.h>
#include <basic-io.h>
#include <limine/deploy.h>
#include <limine/limine.h>
#include <mm/memmap.h>
#include <serial.h>
#include <stdbool.h>

LIMINE_BASE_REF();

void kboot() {
  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hang();
  }

  archEarlyInit();
  serialPuts("Hello, World! From Kernel!\r\n");

  abstractMemmap();
  serialPuts("Something, something\r\n");

  hang();
}