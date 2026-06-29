/*
 * board.h
 * Board support package for clocks, millisecond timing, short blocking delays,
 * and the status LED. Higher layers use these functions instead of touching
 * MSP430 clock and Timer_A registers directly.
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* board_clock_init: raises VCore and configures MCLK/SMCLK from the DCO. */
void board_clock_init(void);

/* board_gpio_init: puts board-level GPIOs into a safe default state. */
void board_gpio_init(void);

/* board_tick_init: starts the 1 ms Timer_A tick used by polling drivers. */
void board_tick_init(void);

/* board_delay_us: busy-waits for us microseconds; us is the requested delay. */
void board_delay_us(uint16_t us);

/* board_delay_ms: busy-waits for ms milliseconds; ms is the requested delay. */
void board_delay_ms(uint16_t ms);

/* board_millis: returns the naturally wrapping millisecond uptime counter. */
uint32_t board_millis(void);

/*
 * board_elapsed_ms: returns 1 when interval_ms elapsed since *stamp_ms and then
 * stores the current time in *stamp_ms; returns 0 otherwise.
 */
uint8_t board_elapsed_ms(uint32_t *stamp_ms, uint16_t interval_ms);

/* board_status_led_set: writes the status LED; on nonzero turns it on. */
void board_status_led_set(uint8_t on);

/* board_status_led_toggle: flips the status LED for heartbeat feedback. */
void board_status_led_toggle(void);

/* board_idle: low-cost idle hook used by the foreground scheduler loop. */
void board_idle(void);

#endif
