#ifndef MACROS_H
#define MACROS_H

#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __naked __attribute__((naked))
#define __naked_section(sec) __attribute__((naked, section(sec)))
#define __bootdata __attribute__((section(".data.boot")))

#endif