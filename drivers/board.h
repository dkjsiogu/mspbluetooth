#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

void board_clock_init(void);
void board_gpio_init(void);
void board_tick_init(void);
void board_delay_us(uint16_t us);
void board_delay_ms(uint16_t ms);
uint32_t board_millis(void);
uint8_t board_elapsed_ms(uint32_t *stamp_ms, uint16_t interval_ms);
void board_status_led_set(uint8_t on);
void board_status_led_toggle(void);
void board_idle(void);

#endif

