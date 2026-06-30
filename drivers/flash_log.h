/*
 * flash_log.h
 * 内部 Flash 历史记录模块。模块把环境采样快照按固定结构写入信息段，
 * 支持上电扫描、追加记录、按序号读取和清空日志。
 */
#ifndef FLASH_LOG_H
#define FLASH_LOG_H

#include <stdint.h>

#include "env_sensors.h"

/* FLASH_LOG_MAGIC: 每条有效历史记录的标记值。 */
#define FLASH_LOG_MAGIC                 0xE551u

/* FlashLogRecord: 写入内部 Flash 的固定长度历史记录。 */
typedef struct {
    uint16_t magic;              /* 有效记录标记。 */
    uint32_t seq;                /* 递增记录序号。 */
    uint32_t uptime_s;           /* 系统运行秒数。 */
    int16_t temperature_x10;     /* 温度，单位 0.1 摄氏度。 */
    uint16_t humidity_x10;       /* 湿度，单位 0.1 %RH。 */
    uint16_t gas_adc;            /* MQ-2 ADC 原始值。 */
    uint16_t distance_mm;        /* 超声波距离，单位 mm。 */
    int16_t threshold_x10;       /* 记录时的温度报警阈值。 */
    uint8_t alarm_level;         /* 记录时的综合报警等级。 */
    uint8_t flags;               /* 传感器有效性状态位。 */
    uint16_t crc;                /* 简单校验和。 */
} FlashLogRecord;

/* flash_log_init: 扫描内部 Flash，恢复历史记录写入位置。 */
void flash_log_init(void);

/*
 * flash_log_append: 追加一条环境历史记录。
 * sample: 当前环境采样。
 * threshold_x10: 当前温度报警阈值。
 * alarm_level: 当前综合报警等级。
 */
void flash_log_append(const EnvSensorSample *sample, int16_t threshold_x10, uint8_t alarm_level);

/* flash_log_clear: 擦除日志使用的信息段并重置序号。 */
void flash_log_clear(void);

/* flash_log_count: 返回当前可读历史记录条数。 */
uint16_t flash_log_count(void);

/*
 * flash_log_read: 按 1 起始序号读取历史记录。
 * index: 第几条历史记录，1 表示最早记录。
 * record: 输出记录指针。
 */
uint8_t flash_log_read(uint16_t index, FlashLogRecord *record);

#endif
