/*
 * alarm_buzzer.c
 * 有源蜂鸣器报警节奏控制。若硬件使用无源蜂鸣器，可把本模块的开关
 * 输出替换为 Timer PWM，占空比仍可沿用当前报警等级策略。
 */
#include "alarm_buzzer.h"

#include <msp430.h>

#include "board.h"
#include "board_pins.h"

/* g_alarm_level: 当前报警等级，0 表示关闭。 */
static uint8_t g_alarm_level = 0u;

/* g_buzzer_on: 当前蜂鸣器输出状态。 */
static uint8_t g_buzzer_on = 0u;

/* g_buzzer_stamp_ms: 上一次蜂鸣器状态切换时间。 */
static uint32_t g_buzzer_stamp_ms = 0u;

/* alarm_period_ms: 报警等级到鸣叫周期的映射。 */
static uint16_t alarm_period_ms(uint8_t level)
{
    static const uint16_t periods[6] = {0u, 1200u, 800u, 500u, 250u, 120u};
    if (level > 5u) {
        level = 5u;
    }
    return periods[level];
}

/* alarm_on_ms: 报警等级到单次响铃时长的映射。 */
static uint16_t alarm_on_ms(uint8_t level)
{
    static const uint16_t widths[6] = {0u, 100u, 120u, 150u, 150u, 80u};
    if (level > 5u) {
        level = 5u;
    }
    return widths[level];
}

/* buzzer_write: 写蜂鸣器 GPIO。 */
static void buzzer_write(uint8_t on)
{
    if (on != 0u) {
        BUZZER_OUT |= BUZZER_BIT;
        g_buzzer_on = 1u;
    } else {
        BUZZER_OUT &= (uint8_t)~BUZZER_BIT;
        g_buzzer_on = 0u;
    }
}

void alarm_buzzer_init(void)
{
    BUZZER_SEL &= (uint8_t)~BUZZER_BIT;
    BUZZER_DIR |= BUZZER_BIT;
    buzzer_write(0u);
    g_buzzer_stamp_ms = board_millis();
}

void alarm_buzzer_set_level(uint8_t level)
{
    if (level > 5u) {
        level = 5u;
    }
    g_alarm_level = level;
    if (g_alarm_level == 0u) {
        buzzer_write(0u);
    }
}

void alarm_buzzer_service(void)
{
    uint16_t elapsed;
    uint16_t period;
    uint16_t on_time;
    uint32_t now;

    if (g_alarm_level == 0u) {
        return;
    }

    now = board_millis();
    elapsed = (uint16_t)(now - g_buzzer_stamp_ms);
    period = alarm_period_ms(g_alarm_level);
    on_time = alarm_on_ms(g_alarm_level);

    if (g_buzzer_on != 0u) {
        if (elapsed >= on_time) {
            buzzer_write(0u);
        }
    } else if (elapsed >= period) {
        g_buzzer_stamp_ms = now;
        buzzer_write(1u);
    }
}
