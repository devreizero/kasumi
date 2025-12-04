#ifndef MACROS_H
#define MACROS_H

#define __rounddown(x, align) ((x) & ~((align) - 1))
#define __roundup(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define __aligndown(x, align) __rounddown(x, align)
#define __alignup(x, align) __roundup(x, align)

#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __naked __attribute__((naked))
#define __naked_section(sec) __attribute__((naked, section(sec)))
#define __initdata __attribute__((section(".data.init")))
#define __init __attribute__((section(".text.init")))

#endif