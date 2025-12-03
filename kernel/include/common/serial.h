#pragma once
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

enum COMBasePort {
    COM1_BASE_PORT = 0x3F8
};

void serialInitPort(uint16_t basePort, uint16_t baudDivisor);
void serialPuts(volatile const char *str);
void serialPutsn(volatile const char *str, uint16_t n);
void serialPutc(volatile char c);

#endif