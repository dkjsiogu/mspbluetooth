/*
 * platform_config.h
 * 多点温度/环境监测系统的集中配置文件。这里统一保存时钟、蓝牙串口、
 * 采样周期、报警阈值和记录容量，避免应用逻辑中散落硬编码参数。
 */
#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <stdint.h>

/* ENV_FIRMWARE_NAME: 蓝牙 info 命令返回的工程名称。 */
#define ENV_FIRMWARE_NAME               "MSP430F5529-ENV-MON"

/* ENV_FIRMWARE_VERSION: 手动维护的软件版本号。 */
#define ENV_FIRMWARE_VERSION            "2.0.0"

/* ENV_HARDWARE_PROFILE: 蓝牙 wiring 命令返回的主要接线摘要。 */
#define ENV_HARDWARE_PROFILE            "DHT:P1.0 MQ2:P6.0 US:P1.2/1.3 OLED:P3.0/3.1 BT:UCA1"

/* MCLK_HZ: CPU 主时钟频率，供延时和时序计算使用。 */
#define MCLK_HZ                         16000000UL

/* SMCLK_HZ: 子系统时钟频率，供 Timer_A 和串口外设使用。 */
#define SMCLK_HZ                        16000000UL

/* BOARD_TICK_HZ: Timer0_A0 产生的系统节拍频率。 */
#define BOARD_TICK_HZ                   1000UL

/* ENV_BT_UART_UCA1_P45: HC-05 使用 UCA1，TX=P4.4，RX=P4.5。 */
#define ENV_BT_UART_UCA1_P45            1

/* ENV_BT_UART_UCA0_P34: HC-05 备用 UCA0，TX=P3.3，RX=P3.4。 */
#define ENV_BT_UART_UCA0_P34            2

/*
 * 课程资料常把 HC-05 接到 P3.3/P3.4。当前默认改用 UCA1 P4.4/P4.5，
 * 这样 P3.0/P3.1 可留给 OLED 软件 I2C，整体接线更清晰。
 */
#define ENV_BT_UART_MODE                ENV_BT_UART_UCA1_P45

/* ENV_SAMPLE_PERIOD_MS: 三类传感器的周期采样间隔。 */
#define ENV_SAMPLE_PERIOD_MS            1000u

/* ENV_OLED_REFRESH_MS: OLED 页面刷新间隔。 */
#define ENV_OLED_REFRESH_MS             500u

/* ENV_BT_PUSH_MS: 蓝牙主动上传实时数据的间隔。 */
#define ENV_BT_PUSH_MS                  2000u

/* ENV_FLASH_LOG_MS: 内部 Flash 历史记录写入间隔。 */
#define ENV_FLASH_LOG_MS                10000u

/* ENV_DEFAULT_TEMP_THRESHOLD_X10: 默认温度报警阈值，单位 0.1 摄氏度。 */
#define ENV_DEFAULT_TEMP_THRESHOLD_X10  300

/* ENV_TEMP_STEP_X10: 编码器和 T+/T- 命令每次调整的阈值步进。 */
#define ENV_TEMP_STEP_X10               5

/* ENV_GAS_WARN_ADC: MQ-2 ADC 原始值超过该值后进入气体报警区。 */
#define ENV_GAS_WARN_ADC                1800u

/* ENV_GAS_STEP_ADC: 气体报警等级每升一级对应的 ADC 增量。 */
#define ENV_GAS_STEP_ADC                300u

/* ENV_TRACE_DEPTH: 近期蓝牙/编码器操作轨迹条数。 */
#define ENV_TRACE_DEPTH                 6u

/* ENCODER_POLL_PERIOD_MS: EC11 正交信号采样周期。 */
#define ENCODER_POLL_PERIOD_MS          2u

/* ENCODER_DEBOUNCE_TICKS: EC11 按键去抖需要的稳定采样次数。 */
#define ENCODER_DEBOUNCE_TICKS          5u

/* ENCODER_LONG_PRESS_MS: EC11 长按事件触发时间。 */
#define ENCODER_LONG_PRESS_MS           800u

/* ENCODER_LONG_PRESS_TICKS: EC11 长按时间折算后的采样次数。 */
#define ENCODER_LONG_PRESS_TICKS        (ENCODER_LONG_PRESS_MS / ENCODER_POLL_PERIOD_MS)

#endif
