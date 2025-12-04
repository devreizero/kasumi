#pragma once
#ifndef STACK_H
#define STACK_H

#include <macros.h>
#include <stdint.h>

extern uintptr_t stackAddress;

__naked uintptr_t getStackAddress();

#endif