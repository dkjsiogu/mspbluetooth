/*
 * encoder.h
 * EC11 rotary encoder driver. The driver polls phase A/B and the integrated
 * push button, debounces them, and exposes semantic events to the player.
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* ENCODER_EVENT_NONE: no new local encoder event is pending. */
#define ENCODER_EVENT_NONE              0x00u

/* ENCODER_EVENT_CW: one clockwise detent was detected. */
#define ENCODER_EVENT_CW                0x01u

/* ENCODER_EVENT_CCW: one counter-clockwise detent was detected. */
#define ENCODER_EVENT_CCW               0x02u

/* ENCODER_EVENT_BUTTON: the EC11 push button was pressed once. */
#define ENCODER_EVENT_BUTTON            0x04u

/* encoder_init: configures EC11 GPIO inputs and initializes debounce state. */
void encoder_init(void);

/* encoder_take_events: polls hardware and returns then clears ENCODER_EVENT_* bits. */
uint8_t encoder_take_events(void);

#endif
