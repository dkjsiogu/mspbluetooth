/*
 * main.c
 * Firmware entry point for the MSP430F5529 Bluetooth WAV player. The file
 * keeps startup order and the foreground scheduler visible in one short place.
 */
#include <msp430.h>

#include "application/audio_player.h"
#include "drivers/bluetooth_uart.h"
#include "drivers/board.h"
#include "drivers/encoder.h"
#include "drivers/i2s_dac.h"
#include "drivers/local_buttons.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    board_clock_init();
    board_gpio_init();
    board_tick_init();
    bluetooth_uart_init();
    encoder_init();
    local_buttons_init();
    i2s_dac_init();

    __enable_interrupt();

    bluetooth_uart_write_line("");
    bluetooth_uart_write_line("MSP430F5529 Bluetooth WAV player");
    audio_player_init();

    while (1) {
        audio_player_poll_controls();
        audio_player_service();
        board_idle();
    }
}
