#ifndef BLUETOOTH_UART_H
#define BLUETOOTH_UART_H

#include <stdint.h>

void bluetooth_uart_init(void);
uint8_t bluetooth_uart_read(uint8_t *byte);
void bluetooth_uart_write_char(uint8_t byte);
void bluetooth_uart_write_str(const char *text);
void bluetooth_uart_write_line(const char *text);
void bluetooth_uart_write_uint(uint32_t value);

#endif

