#pragma once
#ifndef SLUB_H
#define SLUB_H

#include <stddef.h>

void slubInit();
void *slubAlloc(size_t size);
void slubFree(void *paddr);
void slubDumpStats();

#endif