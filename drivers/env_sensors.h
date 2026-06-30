/*
 * env_sensors.h
 * 环境传感器采集驱动。模块负责 DHT11 温湿度、MQ-2 模拟气体电压和
 * HC-SR04 超声波距离采样，并把三类数据整理成统一的样本结构。
 */
#ifndef ENV_SENSORS_H
#define ENV_SENSORS_H

#include <stdint.h>

/* ENV_SENSOR_DHT_OK: DHT11 本次温湿度数据校验通过。 */
#define ENV_SENSOR_DHT_OK               0x01u

/* ENV_SENSOR_DISTANCE_OK: HC-SR04 本次距离测量没有超时。 */
#define ENV_SENSOR_DISTANCE_OK          0x02u

/* ENV_SENSOR_MQ2_READY: MQ-2 已完成预热，可参与气体报警判断。 */
#define ENV_SENSOR_MQ2_READY            0x04u

/* EnvSensorSample: 一次环境采样快照，供显示、蓝牙、存储和报警共用。 */
typedef struct {
    int16_t temperature_x10;    /* 温度，单位 0.1 摄氏度。 */
    uint16_t humidity_x10;      /* 湿度，单位 0.1 %RH。 */
    uint16_t gas_adc;           /* MQ-2 ADC 原始值。 */
    uint8_t gas_level;          /* 气体危险等级，范围 0~5。 */
    uint16_t distance_mm;       /* 超声波距离，单位 mm。 */
    uint8_t flags;              /* ENV_SENSOR_* 状态位。 */
} EnvSensorSample;

/* env_sensors_init: 初始化 DHT11、MQ-2 ADC 和 HC-SR04 相关引脚。 */
void env_sensors_init(void);

/*
 * env_sensors_sample: 执行一次完整环境采样。
 * sample: 输出样本指针，函数会写入温湿度、气体和距离字段。
 */
void env_sensors_sample(EnvSensorSample *sample);

#endif
