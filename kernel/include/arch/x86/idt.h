#ifndef X86_IDT_H
#define X86_IDT_H

#include <stdint.h>

#ifdef ARCH_64

struct InterruptFrame {
    // General-purpose registers pushed by our stub
    unsigned long r11, r10, r9, r8;
    unsigned long rsi, rdi, rdx, ecx, eax;

    // Also pushed by us, but errCode may be CPU-pushed
    unsigned long intNo, errCode;

    // CPU-pushed registers
    unsigned long rip, cs, rflags;
} __attribute__((packed));

struct UserInterruptFrame {
    // General-purpose registers pushed by our stub
    unsigned long r11, r10, r9, r8;
    unsigned long rsi, rdi, rdx, rcx, rax;

    // Also pushed by us, but errCode may be CPU-pushed
    unsigned long intNo, errCode;

    // CPU-pushed registers
    unsigned long rip, cs, rflags, rsp, ss;
} __attribute__((packed));

#else

struct InterruptFrame {
    // General-purpose registers pushed by our stub
    unsigned long edi, esi, ebp, esp_dummy;
    unsigned long ebx, edx, ecx, eax;

    // Also pushed by us, but errCode may be CPU-pushed
    unsigned long intNo, errCode;

    // CPU-pushed registers
    unsigned long eip, cs, eflags;
} __attribute__((packed));

struct UserInterruptFrame {
    // General-purpose registers pushed by our stub
    unsigned long edi, esi, ebp, esp_dummy;
    unsigned long ebx, edx, ecx, eax;

    // Also pushed by us, but errCode may be CPU-pushed
    unsigned long intNo, errCode;

    // CPU-pushed registers
    unsigned long eip, cs, eflags, esp, ss;
} __attribute__((packed));

#endif

void exceptionHandler(struct InterruptFrame *frame);
void irqHandler(struct InterruptFrame *frame); 
void registerInterruptHandler(uint8_t irq, void (*handler)(struct InterruptFrame *));

void initIDT();

#endif