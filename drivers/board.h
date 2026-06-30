/*
 * board.h
 * 板级支持模块。负责时钟、毫秒节拍、短阻塞延时和状态指示灯，高层代码
 * 通过这些函数访问板级能力，避免到处直接操作时钟和 Timer_A 寄存器。
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* board_clock_init: 提升 VCore 并配置 DCO 作为 MCLK/SMCLK。 */
void board_clock_init(void);

/* board_gpio_init: 将板级 GPIO 设置为安全初始状态。 */
void board_gpio_init(void);

/* board_tick_init: 启动 1 ms Timer_A 系统节拍。 */
void board_tick_init(void);

/*
 * board_delay_us: 忙等待指定微秒数。
 * us: 延时时长，单位微秒。
 */
void board_delay_us(uint16_t us);

/*
 * board_delay_ms: 忙等待指定毫秒数。
 * ms: 延时时长，单位毫秒。
 */
void board_delay_ms(uint16_t ms);

/* board_millis: 返回自然回绕的毫秒运行计数。 */
uint32_t board_millis(void);

/*
 * board_elapsed_ms: 到达指定间隔后返回 1，并刷新时间戳。
 * stamp_ms: 上次时间戳指针，间隔到达时被更新。
 * interval_ms: 触发间隔，单位毫秒。
 */
uint8_t board_elapsed_ms(uint32_t *stamp_ms, uint16_t interval_ms);

/*
 * board_status_led_set: 写状态指示灯输出。
 * on: 非零点亮，零熄灭。
 */
void board_status_led_set(uint8_t on);

/* board_status_led_toggle: 翻转状态指示灯，用于心跳反馈。 */
void board_status_led_toggle(void);

/* board_idle: 前台调度循环使用的空闲钩子。 */
void board_idle(void);

#endif
