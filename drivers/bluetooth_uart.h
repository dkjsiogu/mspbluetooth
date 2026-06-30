/*
 * bluetooth_uart.h
 * HC-05 蓝牙串口驱动。模块占用选定的 MSP430 USCI_A 端口，在接收中断
 * 中缓存命令字节，并提供轻量级阻塞发送函数输出状态文本。
 */
#ifndef BLUETOOTH_UART_H
#define BLUETOOTH_UART_H

#include <stdint.h>

/* bluetooth_uart_init: 按 9600-8-N-1 初始化选定 UART，连接 HC-05。 */
void bluetooth_uart_init(void);

/*
 * bluetooth_uart_read: 读取一个已接收字节。
 * byte: 输出指针，接收从环形缓冲区取出的字节。
 */
uint8_t bluetooth_uart_read(uint8_t *byte);

/*
 * bluetooth_uart_write_char: 等待发送缓冲区空闲后发出一个字节。
 * byte: 需要发送到 HC-05 的 ASCII 或二进制字节。
 */
void bluetooth_uart_write_char(uint8_t byte);

/*
 * bluetooth_uart_write_str: 发送零结尾 ASCII 字符串。
 * text: 字符串指针，遇到第一个 '\0' 停止。
 */
void bluetooth_uart_write_str(const char *text);

/*
 * bluetooth_uart_write_line: 发送字符串并追加 CRLF 换行。
 * text: 字符串指针，遇到第一个 '\0' 停止。
 */
void bluetooth_uart_write_line(const char *text);

/*
 * bluetooth_uart_write_uint: 发送无符号十进制整数文本。
 * value: 待格式化的数值，不输出前导零。
 */
void bluetooth_uart_write_uint(uint32_t value);

#endif
