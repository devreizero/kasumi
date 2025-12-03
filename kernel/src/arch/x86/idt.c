#include <stdint.h>

// Architecture selection: define one before compiling
#ifndef ARCH_32
#ifndef ARCH_64
#error "You must define ARCH_32 or ARCH_64"
#endif
#endif

extern void *isrStubTable[32];
extern void *irqStubTable[256 - 32];

// IDT table: 256 entries
#if defined(ARCH_32)
uint64_t idt[256]; // Each entry: 64-bit
#elif defined(ARCH_64)
uint64_t idt[256 * 2]; // Each entry: 128-bit, so 2 uint64_t per entry
#endif

// Helper macros for IDT entry attributes
#define IDT_PRESENT 0x80
#define IDT_RING0 0x00
#define IDT_RING3 0x60
#define IDT_INTERRUPT 0x0E
#define IDT_TRAP 0x0F

// Code segment selector in GDT
#define KERNEL_CS 0x08

// ---------------- IDT Entry Set ----------------
void setIDTEntry(int n, void *handler) {
  uint64_t addr = (uint64_t)handler;

#if defined(ARCH_32)
  uint64_t entry = 0;
  entry |= (addr & 0xFFFFULL);          // Offset bits 0-15
  entry |= ((uint64_t)KERNEL_CS) << 16; // Selector
  entry |= ((uint64_t)(IDT_PRESENT | IDT_RING0 | IDT_INTERRUPT))
           << 40;                            // type_attr
  entry |= ((addr >> 16) & 0xFFFFULL) << 48; // Offset bits 16-31
  idt[n] = entry;

#elif defined(ARCH_64)
  // 64-bit IDT gate: 16 bytes per entry
  uint64_t low = 0, high = 0;
  low |= (addr & 0xFFFFULL);          // Offset 0-15
  low |= ((uint64_t)KERNEL_CS) << 16; // Selector
  low |= ((uint64_t)(IDT_PRESENT | IDT_RING0 | IDT_INTERRUPT))
         << 40;                            // type_attr
  low |= ((addr >> 16) & 0xFFFFULL) << 48; // Offset 16-31

  high |= (addr >> 32) & 0xFFFFFFFFULL; // Offset 32-63
  idt[n * 2] = low;
  idt[n * 2 + 1] = high;
#endif
}

// ---------------- Initialize IDT ----------------
void initIDT() {
  // Fill ISR entries (CPU exceptions)
  for (int i = 0; i < 32; i++) {
    setIDTEntry(i, isrStubTable[i]);
  }

  // Fill IRQ entries (hardware interrupts)
  for (int i = 32; i < 256; i++) {
    setIDTEntry(i, irqStubTable[i - 32]);
  }

  // IDTR structure: limit + base
#ifdef ARCH_32
  struct {
    uint16_t limit;
    uint32_t base;
  } __attribute__((packed)) IDTR;
  IDTR.limit = sizeof(idt) - 1;
  IDTR.base = (uint32_t)&idt;

  asm volatile("lidt %0" : : "m"(IDTR));

#elif defined ARCH_64
  struct {
    uint16_t limit;
    uint64_t base;
  } __attribute__((packed)) IDTR;
  IDTR.limit = sizeof(idt) - 1;
  IDTR.base = (uint64_t)&idt;

  asm volatile("lidt %0" : : "m"(IDTR));
#endif
  __asm__ volatile("sti");
}
