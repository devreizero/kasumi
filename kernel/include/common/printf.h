#pragma once
#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

int vsprintf(char *out, const char *fmt, va_list args);
int fprintf(char *out, const char *fmt, ...);
int printf(const char *fmt, ...);

#define printfOk(...) printf("[   OK] " __VA_ARGS__)
#define printfError(...) printf("[ERROR] " __VA_ARGS__)
#define printfInfo(...) printf("[ INFO] " __VA_ARGS__)
#define printfDebug(...) printf("[DEBUG] " __VA_ARGS__)

#endif