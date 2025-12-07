#ifndef MACROS_H
#define MACROS_H

#define __rounddown(x, align) ((x) & ~((align) - 1))
#define __roundup(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define __aligndown(x, align) __rounddown(x, align)
#define __alignup(x, align) __roundup(x, align)

#define __alignment(align) __attribute__((aligned(align)))

#define SIZE_2KB 0x800
#define SIZE_4KB 0x1000
#define SIZE_2MB 0x200000
#define SIZE_4MB 0x400000
#define SIZE_1GB 0x40000000

#define ALIGN_2KB SIZE_2KB
#define ALIGN_4KB SIZE_4KB
#define ALIGN_2MB SIZE_2MB
#define ALIGN_4MB SIZE_4MB
#define ALIGN_1GB SIZE_1GB

#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __naked __attribute__((naked))
#define __naked_section(sec) __attribute__((naked, section(sec)))
#define __unimplemented(fnName) __attribute__((error(fnName " is not implemented yet")))
#define __initdata __attribute__((section(".data.init")))
#define __init __attribute__((section(".text.init")))

#endif