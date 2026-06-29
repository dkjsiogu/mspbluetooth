/*
 * bluetooth_uart.h
 * HC-05 Bluetooth serial driver. It owns the selected MSP430 USCI_A port,
 * buffers incoming command bytes from the RX ISR, and provides small blocking
 * transmit helpers for status text.
 */
#ifndef BLUETOOTH_UART_H
#define BLUETOOTH_UART_H

#include <stdint.h>

/* bluetooth_uart_init: configures the selected UART for 9600-8-N-1 HC-05 mode. */
void bluetooth_uart_init(void);

/* bluetooth_uart_read: stores the next received byte in *byte; returns 1 if present. */
uint8_t bluetooth_uart_read(uint8_t *byte);

/* bluetooth_uart_write_char: sends one byte; byte is written after TX is ready. */
void bluetooth_uart_write_char(uint8_t byte);

/* bluetooth_uart_write_str: sends a null-terminated ASCII string pointed to by text. */
void bluetooth_uart_write_str(const char *text);

/* bluetooth_uart_write_line: sends text followed by CRLF. */
void bluetooth_uart_write_line(const char *text);

/* bluetooth_uart_write_uint: sends value as an unsigned decimal ASCII number. */
void bluetooth_uart_write_uint(uint32_t value);

#endif
