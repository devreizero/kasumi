#pragma once
#ifndef FORMAT_STRING_H
#define FORMAT_STRING_H

#include <stddef.h>
#include <stdint.h>

enum FormatStringOptions {
    OPTION_NONNULL      = 1 << 0,
    OPTION_NUM_PADZERO  = 1 << 1,
    OPTION_NUM_SIGNED   = 1 << 2,
    OPTION_INT_BASE16   = 1 << 3,
    OPTION_INT_BASE2    = 1 << 4,
};

void formatInteger(signed long num, char buffer[], uint32_t bufLen, uint32_t formatOption);

#endif