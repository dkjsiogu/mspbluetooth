#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <stdint.h>

#define MCLK_HZ                         16000000UL
#define SMCLK_HZ                        16000000UL
#define BOARD_TICK_HZ                   1000UL

#define PLAYER_BT_UART_UCA1_P45         1
#define PLAYER_BT_UART_UCA0_P34         2

/*
 * Course handout lists HC-05 on P3.3/P3.4, but P3.3 is also listed as TF-card
 * MISO. The default build uses UCA1 on P4.4/P4.5 so SD and Bluetooth can both
 * run at the same time. Change to PLAYER_BT_UART_UCA0_P34 only if the TF MISO
 * wiring is moved or Bluetooth TX is intentionally not connected.
 */
#define PLAYER_BT_UART_MODE             PLAYER_BT_UART_UCA1_P45

#define PLAYER_AUTOPLAY_ON_BOOT         1u
#define PLAYER_DEFAULT_VOLUME           18u
#define PLAYER_VOLUME_MAX               32u
#define PLAYER_MAX_TRACKS               9u
#define PLAYER_AUDIO_BUFFER_BYTES       192u

#define ENCODER_POLL_PERIOD_MS          2u
#define ENCODER_DEBOUNCE_TICKS          5u

#endif

