/*
 * alarm_buzzer.h
 * 蜂鸣器报警驱动。模块根据 0~5 级报警等级产生不同急促程度的鸣叫，
 * 等级越高，蜂鸣器响声间隔越短。
 */
#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include <stdint.h>

/* alarm_buzzer_init: 初始化蜂鸣器控制 GPIO，并默认关闭报警。 */
void alarm_buzzer_init(void);

/*
 * alarm_buzzer_set_level: 设置当前报警等级。
 * level: 报警等级，0 关闭，1~5 逐级加急。
 */
void alarm_buzzer_set_level(uint8_t level);

/* alarm_buzzer_service: 按当前等级刷新蜂鸣器开关节奏。 */
void alarm_buzzer_service(void);

#endif
