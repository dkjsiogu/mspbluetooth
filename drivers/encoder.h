/*
 * encoder.h
 * EC11 旋转编码器驱动。模块轮询 A/B 相位和集成按键，完成去抖后向
 * 环境监测应用层提供旋转、短按和长按事件。
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* ENCODER_EVENT_NONE: 当前没有新的编码器事件。 */
#define ENCODER_EVENT_NONE              0x00u

/* ENCODER_EVENT_CW: 检测到一次顺时针旋转。 */
#define ENCODER_EVENT_CW                0x01u

/* ENCODER_EVENT_CCW: 检测到一次逆时针旋转。 */
#define ENCODER_EVENT_CCW               0x02u

/* ENCODER_EVENT_BUTTON: EC11 按键短按一次。 */
#define ENCODER_EVENT_BUTTON            0x04u

/* ENCODER_EVENT_BUTTON_LONG: EC11 按键长按一次。 */
#define ENCODER_EVENT_BUTTON_LONG       0x08u

/* encoder_init: 配置 EC11 GPIO 输入并初始化去抖状态。 */
void encoder_init(void);

/* encoder_take_events: 轮询硬件，返回并清除 ENCODER_EVENT_* 事件位。 */
uint8_t encoder_take_events(void);

#endif
