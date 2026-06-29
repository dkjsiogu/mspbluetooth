/*
 * encoder.c
 * EC11 rotary encoder implementation. It polls quadrature phase transitions and
 * the active-low push switch, then exposes debounced semantic events.
 */
#include "encoder.h"

#include <msp430.h>

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

/* g_last_ab: previous two-bit EC11 A/B phase sample. */
static uint8_t g_last_ab = 0;

/* g_quadrature_accum: signed transition accumulator; +/-4 equals one detent. */
static int8_t g_quadrature_accum = 0;

/* g_pending_events: accumulated ENCODER_EVENT_* bits since the last read. */
static uint8_t g_pending_events = ENCODER_EVENT_NONE;

/* g_button_sample: most recent raw EC11 push-button sample. */
static uint8_t g_button_sample = 0;

/* g_button_stable: debounced EC11 push-button state. */
static uint8_t g_button_stable = 0;

/* g_button_ticks: count of consecutive samples matching g_button_sample. */
static uint8_t g_button_ticks = 0;

/* g_button_hold_ticks: stable pressed samples counted toward long press. */
static uint16_t g_button_hold_ticks = 0u;

/* g_button_long_sent: nonzero after the current press emitted a long event. */
static uint8_t g_button_long_sent = 0u;

/* g_last_poll_ms: millisecond timestamp used to pace encoder polling. */
static uint32_t g_last_poll_ms = 0;

/* encoder_read_ab: returns phase A in bit0 and phase B in bit1. */
static uint8_t encoder_read_ab(void)
{
    uint8_t value;

    value = 0u;
    if ((ENC_IN & ENC_A_BIT) != 0u) {
        value |= 0x01u;
    }
    if ((ENC_IN & ENC_B_BIT) != 0u) {
        value |= 0x02u;
    }

    return value;
}

/* encoder_read_button_pressed: returns 1 while the active-low EC11 switch is down. */
static uint8_t encoder_read_button_pressed(void)
{
    return (uint8_t)((ENC_IN & ENC_SW_BIT) == 0u);
}

/* encoder_reset_button_hold: clears hold tracking for a newly pressed switch. */
static void encoder_reset_button_hold(void)
{
    g_button_hold_ticks = 0u;
    g_button_long_sent = 0u;
}

/* encoder_update_button_hold: emits one long-press event after the hold threshold. */
static void encoder_update_button_hold(void)
{
    if ((g_button_stable == 0u) || (g_button_long_sent != 0u)) {
        return;
    }

    if (g_button_hold_ticks < ENCODER_LONG_PRESS_TICKS) {
        g_button_hold_ticks++;
    }

    if (g_button_hold_ticks >= ENCODER_LONG_PRESS_TICKS) {
        g_pending_events |= ENCODER_EVENT_BUTTON_LONG;
        g_button_long_sent = 1u;
    }
}

void encoder_init(void)
{
    ENC_SEL &= (uint8_t)~(ENC_A_BIT | ENC_B_BIT | ENC_SW_BIT);
    ENC_DIR &= (uint8_t)~(ENC_A_BIT | ENC_B_BIT | ENC_SW_BIT);
    ENC_REN |= ENC_A_BIT | ENC_B_BIT | ENC_SW_BIT;
    ENC_OUT |= ENC_A_BIT | ENC_B_BIT | ENC_SW_BIT;

    g_last_ab = encoder_read_ab();
    g_button_sample = encoder_read_button_pressed();
    g_button_stable = g_button_sample;
    g_button_ticks = 0u;
    encoder_reset_button_hold();
}

/* encoder_poll: samples A/B/SW, decodes movement, and debounces the button. */
static void encoder_poll(void)
{
    /* transition_table: signed quadrature delta indexed by previous/current AB. */
    static const int8_t transition_table[16] = {
        0, -1,  1,  0,
        1,  0,  0, -1,
       -1,  0,  0,  1,
        0,  1, -1,  0
    };
    uint8_t now_ab;
    uint8_t index;
    int8_t step;
    uint8_t button_now;

    if (board_elapsed_ms(&g_last_poll_ms, ENCODER_POLL_PERIOD_MS) == 0u) {
        return;
    }

    now_ab = encoder_read_ab();
    index = (uint8_t)((g_last_ab << 2) | now_ab);
    step = transition_table[index & 0x0Fu];
    g_last_ab = now_ab;

    if (step != 0) {
        g_quadrature_accum = (int8_t)(g_quadrature_accum + step);
        if (g_quadrature_accum >= 4) {
            g_pending_events |= ENCODER_EVENT_CW;
            g_quadrature_accum = 0;
        } else if (g_quadrature_accum <= -4) {
            g_pending_events |= ENCODER_EVENT_CCW;
            g_quadrature_accum = 0;
        }
    }

    button_now = encoder_read_button_pressed();
    if (button_now != g_button_sample) {
        g_button_sample = button_now;
        g_button_ticks = 0u;
    } else if (g_button_ticks < ENCODER_DEBOUNCE_TICKS) {
        g_button_ticks++;
        if ((g_button_ticks >= ENCODER_DEBOUNCE_TICKS) &&
            (g_button_stable != g_button_sample)) {
            g_button_stable = g_button_sample;
            if (g_button_stable != 0u) {
                encoder_reset_button_hold();
            } else if (g_button_long_sent == 0u) {
                g_pending_events |= ENCODER_EVENT_BUTTON;
            }
        }
    } else {
        encoder_update_button_hold();
    }
}

uint8_t encoder_take_events(void)
{
    uint8_t events;

    encoder_poll();
    events = g_pending_events;
    g_pending_events = ENCODER_EVENT_NONE;
    return events;
}
