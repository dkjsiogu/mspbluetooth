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

/*
 * bluetooth_uart_read: stores the next received byte and returns 1 if present.
 * byte: output pointer that receives the byte removed from the RX ring buffer.
 */
uint8_t bluetooth_uart_read(uint8_t *byte);

/*
 * bluetooth_uart_write_char: sends one byte after the UART TX buffer is ready.
 * byte: ASCII or binary byte to transmit to the HC-05 module.
 */
void bluetooth_uart_write_char(uint8_t byte);

/*
 * bluetooth_uart_write_str: sends a null-terminated ASCII string.
 * text: string pointer; transmission stops at the first '\0'.
 */
void bluetooth_uart_write_str(const char *text);

/*
 * bluetooth_uart_write_line: sends a string followed by CRLF.
 * text: string pointer; transmission stops at the first '\0'.
 */
void bluetooth_uart_write_line(const char *text);

/*
 * bluetooth_uart_write_uint: sends an unsigned decimal ASCII number.
 * value: numeric value to format without leading zeroes.
 */
void bluetooth_uart_write_uint(uint32_t value);

#endif
