#include <arch-hook.h>
#include <serial.h>
#include <x86/idt.h>

void archEarlyInit() {
    initIDT();
    serialInitPort(COM1_BASE_PORT, 9600);
}