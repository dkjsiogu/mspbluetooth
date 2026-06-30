/*
 * encoder.c
 * EC11 旋转编码器实现。模块轮询正交相位变化和低电平有效按键，并输出
 * 已去抖的语义事件。
 */
#include "encoder.h"

#include <msp430.h>

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

/* g_last_ab: 上一次 EC11 A/B 两位相位采样。 */
static uint8_t g_last_ab = 0;

/* g_quadrature_accum: 正交相位累加器，累计到 +/-4 视为一格。 */
static int8_t g_quadrature_accum = 0;

/* g_pending_events: 上次读取后累计的 ENCODER_EVENT_* 事件位。 */
static uint8_t g_pending_events = ENCODER_EVENT_NONE;

/* g_button_sample: 最近一次 EC11 按键原始采样。 */
static uint8_t g_button_sample = 0;

/* g_button_stable: 去抖后的 EC11 按键稳定状态。 */
static uint8_t g_button_stable = 0;

/* g_button_ticks: 与当前原始采样连续一致的次数。 */
static uint8_t g_button_ticks = 0;

/* g_button_hold_ticks: 稳定按下后累计的长按采样次数。 */
static uint16_t g_button_hold_ticks = 0u;

/* g_button_long_sent: 当前按压周期是否已经发出长按事件。 */
static uint8_t g_button_long_sent = 0u;

/* g_last_poll_ms: 控制编码器轮询周期的毫秒时间戳。 */
static uint32_t g_last_poll_ms = 0;

/* encoder_read_ab: 读取 A/B 相位，A 放 bit0，B 放 bit1。 */
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

/* encoder_read_button_pressed: EC11 低电平按下时返回 1。 */
static uint8_t encoder_read_button_pressed(void)
{
    return (uint8_t)((ENC_IN & ENC_SW_BIT) == 0u);
}

/* encoder_reset_button_hold: 新一轮按压开始时清空长按跟踪状态。 */
static void encoder_reset_button_hold(void)
{
    g_button_hold_ticks = 0u;
    g_button_long_sent = 0u;
}

/* encoder_update_button_hold: 稳定按压超过阈值后只发出一次长按事件。 */
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

/* encoder_poll: 采样 A/B/SW，解码旋转方向并处理按键去抖。 */
static void encoder_poll(void)
{
    /* transition_table: 以前后 AB 状态为索引的正交解码增量表。 */
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
