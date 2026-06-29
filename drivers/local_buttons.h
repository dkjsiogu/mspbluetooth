/*
 * local_buttons.h
 * Debounced local push-button input for the board. These buttons supplement
 * Bluetooth and EC11 control without changing the core player state machine.
 */
#ifndef LOCAL_BUTTONS_H
#define LOCAL_BUTTONS_H

#include <stdint.h>

/* LOCAL_BUTTON_EVENT_NONE: no debounced local button press is pending. */
#define LOCAL_BUTTON_EVENT_NONE         0x00u

/* LOCAL_BUTTON_EVENT_PLAY_PAUSE: S1 press, mapped to play/pause. */
#define LOCAL_BUTTON_EVENT_PLAY_PAUSE   0x01u

/* LOCAL_BUTTON_EVENT_PREVIOUS: S2 press, mapped to previous track. */
#define LOCAL_BUTTON_EVENT_PREVIOUS     0x02u

/* LOCAL_BUTTON_EVENT_NEXT: S4 press, mapped to next track. */
#define LOCAL_BUTTON_EVENT_NEXT         0x04u

/* local_buttons_init: configures S1/S2/S4 GPIO inputs with pull-ups. */
void local_buttons_init(void);

/* local_buttons_take_events: polls buttons and returns then clears LOCAL_BUTTON_EVENT_* bits. */
uint8_t local_buttons_take_events(void);

#endif
