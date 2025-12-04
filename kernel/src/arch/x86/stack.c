#include <macros.h>
#include <stack.h>
#include <stdint.h>

__naked uintptr_t getStackAddress() {
    __asm__ __volatile__(
        "mov %rsp, %rax\n"
        "ret\n"
    );
}