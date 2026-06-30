/*
 * bluetooth_uart.c
 * HC-05 串口驱动实现。接收中断把蓝牙命令字节写入环形缓冲区，前台
 * 环境监测逻辑使用阻塞式发送函数输出文本状态。
 */
#include "bluetooth_uart.h"

#include <msp430.h>

#include "board_pins.h"
#include "platform_config.h"

/* UART_RX_BUFFER_SIZE: 蓝牙接收环形缓冲区长度，必须为 2 的幂。 */
#define UART_RX_BUFFER_SIZE             32u

/* UART_RX_BUFFER_MASK: 环形缓冲区下标回绕掩码。 */
#define UART_RX_BUFFER_MASK             (UART_RX_BUFFER_SIZE - 1u)

/* g_rx_buffer: 接收中断写入的命令字节环形缓冲区。 */
static volatile uint8_t g_rx_buffer[UART_RX_BUFFER_SIZE];

/* g_rx_head: 接收中断推进的写入下标。 */
static volatile uint8_t g_rx_head = 0;

/* g_rx_tail: 前台命令轮询推进的读取下标。 */
static volatile uint8_t g_rx_tail = 0;

/* uart_rx_push: 在缓冲区未满时保存一个接收字节。 */
static void uart_rx_push(uint8_t byte)
{
    uint8_t next;

    next = (uint8_t)((g_rx_head + 1u) & UART_RX_BUFFER_MASK);
    if (next != g_rx_tail) {
        g_rx_buffer[g_rx_head] = byte;
        g_rx_head = next;
    }
}

uint8_t bluetooth_uart_read(uint8_t *byte)
{
    if (g_rx_head == g_rx_tail) {
        return 0u;
    }

    *byte = g_rx_buffer[g_rx_tail];
    g_rx_tail = (uint8_t)((g_rx_tail + 1u) & UART_RX_BUFFER_MASK);
    return 1u;
}

void bluetooth_uart_init(void)
{
#if ENV_BT_UART_MODE == ENV_BT_UART_UCA1_P45
    BT_UCA1_SEL |= BT_UCA1_TX_BIT | BT_UCA1_RX_BIT;
    BT_UCA1_DIR |= BT_UCA1_TX_BIT;
    BT_UCA1_DIR &= (uint8_t)~BT_UCA1_RX_BIT;

    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 = UCSWRST | UCSSEL_2;
    UCA1BR0 = 104u;
    UCA1BR1 = 0u;
    UCA1MCTL = UCBRF_3 | UCBRS_0 | UCOS16;
    UCA1CTL1 &= (uint8_t)~UCSWRST;
    UCA1IE |= UCRXIE;
#elif ENV_BT_UART_MODE == ENV_BT_UART_UCA0_P34
    BT_UCA0_SEL |= BT_UCA0_TX_BIT | BT_UCA0_RX_BIT;
    BT_UCA0_DIR |= BT_UCA0_TX_BIT;
    BT_UCA0_DIR &= (uint8_t)~BT_UCA0_RX_BIT;

    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 = UCSWRST | UCSSEL_2;
    UCA0BR0 = 104u;
    UCA0BR1 = 0u;
    UCA0MCTL = UCBRF_3 | UCBRS_0 | UCOS16;
    UCA0CTL1 &= (uint8_t)~UCSWRST;
    UCA0IE |= UCRXIE;
#else
#error Unsupported ENV_BT_UART_MODE
#endif
}

void bluetooth_uart_write_char(uint8_t byte)
{
#if ENV_BT_UART_MODE == ENV_BT_UART_UCA1_P45
    while ((UCA1IFG & UCTXIFG) == 0u) {
    }
    UCA1TXBUF = byte;
#else
    while ((UCA0IFG & UCTXIFG) == 0u) {
    }
    UCA0TXBUF = byte;
#endif
}

void bluetooth_uart_write_str(const char *text)
{
    while (*text != '\0') {
        bluetooth_uart_write_char((uint8_t)*text);
        text++;
    }
}

void bluetooth_uart_write_line(const char *text)
{
    bluetooth_uart_write_str(text);
    bluetooth_uart_write_str("\r\n");
}

void bluetooth_uart_write_uint(uint32_t value)
{
    char digits[10];
    uint8_t count;

    count = 0u;
    if (value == 0u) {
        bluetooth_uart_write_char((uint8_t)'0');
        return;
    }

    while ((value > 0u) && (count < (uint8_t)sizeof(digits))) {
        digits[count] = (char)('0' + (char)(value % 10u));
        value /= 10u;
        count++;
    }

    while (count > 0u) {
        count--;
        bluetooth_uart_write_char((uint8_t)digits[count]);
    }
}

#if ENV_BT_UART_MODE == ENV_BT_UART_UCA1_P45
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    switch (__even_in_range(UCA1IV, 4)) {
    case 2:
        uart_rx_push(UCA1RXBUF);
        __bic_SR_register_on_exit(LPM0_bits);
        break;
    default:
        break;
    }
}
#else
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, 4)) {
    case 2:
        uart_rx_push(UCA0RXBUF);
        __bic_SR_register_on_exit(LPM0_bits);
        break;
    default:
        break;
    }
}
#endif
