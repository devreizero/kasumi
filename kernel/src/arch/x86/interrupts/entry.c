#include <stddef.h>
#include <stdint.h>

#include <basic-io.h>
#include <x86/idt.h>

enum IntNo {
	GENERAL_PROTECTION = 13,
	PAGE_FAULT = 14
};

void (*interruptHandlers[256])(struct InterruptFrame *frame);

void registerInterruptHandler(uint8_t interrupt, void (*handler)(struct InterruptFrame *frame)) {
    interruptHandlers[interrupt] = handler;
}

void exceptionHandler(struct InterruptFrame *frame) {
	static int isDead = 0;
	uint64_t cr2, rsp;
	__asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
	__asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));

	hang();
}

void irqHandler(struct InterruptFrame *frame) {
	if (interruptHandlers[frame->intNo] != NULL) {
		interruptHandlers[frame->intNo](frame);
	}
}