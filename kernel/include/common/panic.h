#pragma once
#ifndef PANIC_H
#define PANIC_H

#include <macros.h>

#define panic(...) __panicInternal(__FILE_NAME__, __LINE__, __VA_ARGS__)

__noreturn void __panicInternal(const char *filename, unsigned int line, const char *msg, ...);

#endif