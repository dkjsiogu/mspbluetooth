#include "bluetooth_uart.h"

#include <msp430.h>

#include "board_pins.h"
#include "platform_config.h"

/* UART_RX_BUFFER_SIZE: power-of-two RX ring size for Bluetooth command bytes. */
#define UART_RX_BUFFER_SIZE             32u

/* UART_RX_BUFFER_MASK: wraps RX ring indices without division. */
#define UART_RX_BUFFER_MASK             (UART_RX_BUFFER_SIZE - 1u)

/* g_rx_buffer: interrupt-filled circular buffer for received command bytes. */
static volatile uint8_t g_rx_buffer[UART_RX_BUFFER_SIZE];

/* g_rx_head: write index advanced by the UART RX ISR. */
static volatile uint8_t g_rx_head = 0;

/* g_rx_tail: read index advanced by foreground command polling. */
static volatile uint8_t g_rx_tail = 0;

/* uart_rx_push: stores byte in the RX ring unless it is full. */
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
#if PLAYER_BT_UART_MODE == PLAYER_BT_UART_UCA1_P45
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
#elif PLAYER_BT_UART_MODE == PLAYER_BT_UART_UCA0_P34
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
#error Unsupported PLAYER_BT_UART_MODE
#endif
}

void bluetooth_uart_write_char(uint8_t byte)
{
#if PLAYER_BT_UART_MODE == PLAYER_BT_UART_UCA1_P45
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

#if PLAYER_BT_UART_MODE == PLAYER_BT_UART_UCA1_P45
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
