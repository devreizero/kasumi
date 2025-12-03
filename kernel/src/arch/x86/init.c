#include <arch-hook.h>
#include <serial.h>
#include <x86/idt.h>

void archEarlyInit() {
    initIDT();
}