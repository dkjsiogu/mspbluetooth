#include "local_buttons.h"

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

/* ButtonDebounce: compact per-button state for active-low sampled inputs. */
typedef struct {
    uint8_t sample;
    uint8_t stable;
    uint8_t ticks;
} ButtonDebounce;

/* g_button1: debounce state for S1, mapped to play/pause. */
static ButtonDebounce g_button1;

/* g_button2: debounce state for S2, mapped to previous track. */
static ButtonDebounce g_button2;

/* g_button4: debounce state for S4, mapped to next track. */
static ButtonDebounce g_button4;

/* g_pending_events: accumulated LOCAL_BUTTON_EVENT_* bits since the last read. */
static uint8_t g_pending_events = LOCAL_BUTTON_EVENT_NONE;

/* g_last_poll_ms: millisecond timestamp used to pace the button scanner. */
static uint32_t g_last_poll_ms = 0u;

/* button_update: debounces one button and emits event_bit on a new press. */
static void button_update(ButtonDebounce *button, uint8_t pressed, uint8_t event_bit)
{
    if (pressed != button->sample) {
        button->sample = pressed;
        button->ticks = 0u;
        return;
    }

    if (button->ticks < LOCAL_BUTTON_DEBOUNCE_TICKS) {
        button->ticks++;
        if ((button->ticks >= LOCAL_BUTTON_DEBOUNCE_TICKS) &&
            (button->stable != button->sample)) {
            button->stable = button->sample;
            if (button->stable != 0u) {
                g_pending_events |= event_bit;
            }
        }
    }
}

/* read_button1: returns 1 while S1 is pressed, otherwise 0. */
static uint8_t read_button1(void)
{
    return (uint8_t)((LOCAL_BTN1_IN & LOCAL_BTN1_BIT) == 0u);
}

/* read_button2: returns 1 while S2 is pressed, otherwise 0. */
static uint8_t read_button2(void)
{
    return (uint8_t)((LOCAL_BTN2_IN & LOCAL_BTN2_BIT) == 0u);
}

/* read_button4: returns 1 while S4 is pressed, otherwise 0. */
static uint8_t read_button4(void)
{
    return (uint8_t)((LOCAL_BTN4_IN & LOCAL_BTN4_BIT) == 0u);
}

void local_buttons_init(void)
{
    LOCAL_BTN1_SEL &= (uint8_t)~LOCAL_BTN1_BIT;
    LOCAL_BTN1_DIR &= (uint8_t)~LOCAL_BTN1_BIT;
    LOCAL_BTN1_REN |= LOCAL_BTN1_BIT;
    LOCAL_BTN1_OUT |= LOCAL_BTN1_BIT;

    LOCAL_BTN2_SEL &= (uint8_t)~LOCAL_BTN2_BIT;
    LOCAL_BTN2_DIR &= (uint8_t)~LOCAL_BTN2_BIT;
    LOCAL_BTN2_REN |= LOCAL_BTN2_BIT;
    LOCAL_BTN2_OUT |= LOCAL_BTN2_BIT;

    LOCAL_BTN4_SEL &= (uint8_t)~LOCAL_BTN4_BIT;
    LOCAL_BTN4_DIR &= (uint8_t)~LOCAL_BTN4_BIT;
    LOCAL_BTN4_REN |= LOCAL_BTN4_BIT;
    LOCAL_BTN4_OUT |= LOCAL_BTN4_BIT;

    g_button1.sample = read_button1();
    g_button1.stable = g_button1.sample;
    g_button1.ticks = 0u;
    g_button2.sample = read_button2();
    g_button2.stable = g_button2.sample;
    g_button2.ticks = 0u;
    g_button4.sample = read_button4();
    g_button4.stable = g_button4.sample;
    g_button4.ticks = 0u;
}

uint8_t local_buttons_take_events(void)
{
    uint8_t events;

    if (board_elapsed_ms(&g_last_poll_ms, LOCAL_BUTTON_POLL_PERIOD_MS) != 0u) {
        button_update(&g_button1, read_button1(), LOCAL_BUTTON_EVENT_PLAY_PAUSE);
        button_update(&g_button2, read_button2(), LOCAL_BUTTON_EVENT_PREVIOUS);
        button_update(&g_button4, read_button4(), LOCAL_BUTTON_EVENT_NEXT);
    }

    events = g_pending_events;
    g_pending_events = LOCAL_BUTTON_EVENT_NONE;
    return events;
}
