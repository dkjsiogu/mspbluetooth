#include "encoder.h"

#include <msp430.h>

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

static uint8_t g_last_ab = 0;
static int8_t g_quadrature_accum = 0;
static uint8_t g_pending_events = ENCODER_EVENT_NONE;
static uint8_t g_button_sample = 0;
static uint8_t g_button_stable = 0;
static uint8_t g_button_ticks = 0;
static uint32_t g_last_poll_ms = 0;

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

static uint8_t encoder_read_button_pressed(void)
{
    return (uint8_t)((ENC_IN & ENC_SW_BIT) == 0u);
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
}

static void encoder_poll(void)
{
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
                g_pending_events |= ENCODER_EVENT_BUTTON;
            }
        }
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

