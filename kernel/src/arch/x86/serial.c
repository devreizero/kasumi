#include <serial.h>
#include <stdint.h>
#include <x86/port-io.h>

void serialInitPort(uint16_t basePort, uint16_t baudDivisor) {
        // Disable UART interrupts during initialization to prevent unexpected behavior.
    outb(basePort + 1, 0x00);                 // Disable all interrupts

    // Set baud rate by enabling Divisor Latch Access Bit (DLAB) in LCR (COM1_BASE_PORT + 3)
    // This gives access to Divisor Latch Low (DLL) and High (DLM) registers
    // Divisor = UART_CLOCK / baud_rate (UART_CLOCK typically 115200 Hz)
    // Example: for 9600 baud, divisor = 115200 / 9600 = 12
    outb(basePort + 3, 0x80);                                     // Enable DLAB
    outb(basePort + 0, (uint8_t)(baudDivisor & 0xFF));            // DLL low byte
    outb(basePort + 1, (uint8_t)((baudDivisor >> 8) & 0xFF));     // DLM high byte

    // Disable DLAB and configure 8 data bits, no parity, 1 stop bit.
    outb(basePort + 3, 0x03);

    // Configure FIFO: enable, clear, and set 14-byte threshold.
    // Enabling and clearing them with a threshold can improve performance.
    outb(basePort + 2, 0xC7);

    // Configure Modem Control: set RTS/DTR and enable UART interrupts.
    outb(basePort + 4, 0x0B);
}

void serialPuts(volatile const char *string) {
    while (*string) {
        serialPutc(*string);
        string++;
    }
}

void serialPutsn(volatile const char *string, uint16_t n) {
    // Log even past null bytes, we must believe in `length`.
    for (uint16_t i = 0; i < n; i++) {
        serialPutc(string[i]);
    }
}

void serialPutc(volatile char character) {
    // Wait until the Transmitter Holding Register (THR) is empty (THRE bit in LSR).
    while (!((inb(COM1_BASE_PORT + 5)) & 0x20));
    outb(COM1_BASE_PORT, character); // Write the character to the Data Register (COM1_BASE_PORT).
}