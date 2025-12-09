#include <basic_io.h>
#include <macros.h>

void hang() {
    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}