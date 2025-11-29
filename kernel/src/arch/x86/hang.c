#include "basic-io.h"

void hang() {
    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}