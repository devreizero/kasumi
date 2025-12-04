#include <format_string.h>
#include <stdint.h>

void formatInteger(signed long num, char buffer[], uint32_t bufLen, uint32_t formatOption) {
    static const char letters[17] = "0123456789abcdef";
    int base = 10;
    if ((formatOption & OPTION_INT_BASE16)) {
        base = 16;
    } else if ((formatOption & OPTION_INT_BASE2)) {
        base = 2;
    }

    if (bufLen == 0) return; // GET OUT!!!

    // Null-terminate if there is no OPTION_NONNULL
    if ((formatOption & OPTION_NONNULL) == 0) buffer[--bufLen] = '\0';

    int isNegative = 0;
    unsigned long absNum = (unsigned long)num;
    // Handle signed option
    if ((formatOption & OPTION_NUM_SIGNED) && num < 0) {
        isNegative = 1;
        absNum = (unsigned long) (-num);
    }

    // Handle 0 explicitly
    if (absNum == 0) {
        if (bufLen > 1) buffer[0] = '0';
        return;
    }

    int32_t pos = bufLen - 1; // Start writing from tail
    while (absNum > 0 && pos >= 0) {
        int digit = absNum % base;
        buffer[pos--] = letters[digit];
        absNum /= base;
    }

    // Add negative sign if needed
    if (isNegative && pos >= 0) {
        buffer[pos--] = '-';
    }

    // Pad with 0 or move the data
    int start = pos + 1;
    int i = 0;
    if ((formatOption & OPTION_NUM_PADZERO)) {
        while (start < (int) bufLen - 1 && i < (int) (bufLen - 1)) {
            buffer[i++] = '0';
            start++;
        }
    } else {
        while (start < (int) bufLen - 1 && i < (int) (bufLen - 1)) {
            buffer[i++] = buffer[start++];
        }
    }
    buffer[i] = '\0';
}
