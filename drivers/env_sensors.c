/*
 * env_sensors.c
 * DHT11、MQ-2 和 HC-SR04 的底层采样实现。DHT11 与超声波使用软件
 * 微秒级轮询，MQ-2 使用 ADC12 单通道多次采样求平均。
 */
#include "env_sensors.h"

#include <msp430.h>

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

/* DHT_TIMEOUT_US: DHT11 单个电平等待的最长时间，防止传感器异常卡死。 */
#define DHT_TIMEOUT_US                  120u

/* HCSR04_TIMEOUT_US: 超声波 Echo 等待超时，约等于 5 米以内测距。 */
#define HCSR04_TIMEOUT_US               30000u

/* MQ2_WARMUP_MS: MQ-2 上电预热时间，预热前只显示数值不参与报警。 */
#define MQ2_WARMUP_MS                   30000UL

/* g_sensor_start_ms: 传感器初始化时间，用于判断 MQ-2 是否预热完成。 */
static uint32_t g_sensor_start_ms = 0;

/* dht_release_line: 释放 DHT11 DATA，让上拉电阻把总线拉高。 */
static void dht_release_line(void)
{
    DHT11_DIR &= (uint8_t)~DHT11_BIT;
    DHT11_REN |= DHT11_BIT;
    DHT11_OUT |= DHT11_BIT;
}

/* dht_drive_low: 主控主动拉低 DHT11 DATA。 */
static void dht_drive_low(void)
{
    DHT11_DIR |= DHT11_BIT;
    DHT11_OUT &= (uint8_t)~DHT11_BIT;
}

/* dht_wait_level: 等待 DHT11 DATA 变成目标电平。 */
static uint8_t dht_wait_level(uint8_t high)
{
    uint16_t elapsed;

    for (elapsed = 0u; elapsed < DHT_TIMEOUT_US; elapsed++) {
        if ((((DHT11_IN & DHT11_BIT) != 0u) ? 1u : 0u) == high) {
            return 1u;
        }
        board_delay_us(1u);
    }

    return 0u;
}

/* dht_measure_high_us: 统计 DHT11 一个数据位的高电平持续时间。 */
static uint16_t dht_measure_high_us(void)
{
    uint16_t width;

    width = 0u;
    while ((DHT11_IN & DHT11_BIT) != 0u) {
        if (width >= DHT_TIMEOUT_US) {
            break;
        }
        board_delay_us(1u);
        width++;
    }
    return width;
}

/* dht_read: 读取 DHT11 并输出 0.1 单位温湿度。 */
static uint8_t dht_read(int16_t *temperature_x10, uint16_t *humidity_x10)
{
    uint8_t data[5];
    uint8_t byte_index;
    uint8_t bit_index;
    uint8_t sum;
    uint16_t high_us;
    uint16_t gie;

    for (byte_index = 0u; byte_index < 5u; byte_index++) {
        data[byte_index] = 0u;
    }

    gie = (uint16_t)(__get_SR_register() & GIE);
    __disable_interrupt();

    dht_drive_low();
    board_delay_ms(20u);
    dht_release_line();
    board_delay_us(30u);

    if ((dht_wait_level(0u) == 0u) || (dht_wait_level(1u) == 0u) ||
        (dht_wait_level(0u) == 0u)) {
        if (gie != 0u) {
            __enable_interrupt();
        }
        return 0u;
    }

    for (byte_index = 0u; byte_index < 5u; byte_index++) {
        for (bit_index = 0u; bit_index < 8u; bit_index++) {
            if (dht_wait_level(1u) == 0u) {
                if (gie != 0u) {
                    __enable_interrupt();
                }
                return 0u;
            }
            high_us = dht_measure_high_us();
            data[byte_index] <<= 1;
            if (high_us > 45u) {
                data[byte_index] |= 1u;
            }
        }
    }

    if (gie != 0u) {
        __enable_interrupt();
    }

    sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) {
        return 0u;
    }

    *humidity_x10 = (uint16_t)data[0] * 10u + data[1];
    *temperature_x10 = (int16_t)((int16_t)data[2] * 10 + (int16_t)data[3]);
    return 1u;
}

/* mq2_read_adc: 对 MQ-2 ADC 通道做多次采样并返回平均值。 */
static uint16_t mq2_read_adc(void)
{
    uint8_t index;
    uint32_t total;

    total = 0u;
    for (index = 0u; index < 8u; index++) {
        ADC12CTL0 &= (uint16_t)~ADC12ENC;
        ADC12MCTL0 = ADC12INCH_0;
        ADC12CTL0 |= ADC12ENC | ADC12SC;
        while ((ADC12IFG & ADC12IFG0) == 0u) {
        }
        total += ADC12MEM0;
    }

    return (uint16_t)(total / 8u);
}

/* mq2_level: 把 MQ-2 ADC 原始值换算为 0~5 危险等级。 */
static uint8_t mq2_level(uint16_t adc)
{
    uint16_t over;
    uint8_t level;

    if (adc <= ENV_GAS_WARN_ADC) {
        return 0u;
    }

    over = (uint16_t)(adc - ENV_GAS_WARN_ADC);
    level = (uint8_t)(1u + (over / ENV_GAS_STEP_ADC));
    if (level > 5u) {
        level = 5u;
    }
    return level;
}

/* hcsr04_read_mm: 触发 HC-SR04 并返回毫米距离，失败返回 0。 */
static uint8_t hcsr04_read_mm(uint16_t *distance_mm)
{
    uint16_t wait_us;
    uint16_t echo_us;

    HCSR04_TRIG_OUT &= (uint8_t)~HCSR04_TRIG_BIT;
    board_delay_us(2u);
    HCSR04_TRIG_OUT |= HCSR04_TRIG_BIT;
    board_delay_us(10u);
    HCSR04_TRIG_OUT &= (uint8_t)~HCSR04_TRIG_BIT;

    for (wait_us = 0u; wait_us < HCSR04_TIMEOUT_US; wait_us++) {
        if ((HCSR04_ECHO_IN & HCSR04_ECHO_BIT) != 0u) {
            break;
        }
        board_delay_us(1u);
    }
    if (wait_us >= HCSR04_TIMEOUT_US) {
        return 0u;
    }

    echo_us = 0u;
    while ((HCSR04_ECHO_IN & HCSR04_ECHO_BIT) != 0u) {
        if (echo_us >= HCSR04_TIMEOUT_US) {
            return 0u;
        }
        board_delay_us(1u);
        echo_us++;
    }

    *distance_mm = (uint16_t)(((uint32_t)echo_us * 10u) / 58u);
    return 1u;
}

void env_sensors_init(void)
{
    DHT11_SEL &= (uint8_t)~DHT11_BIT;
    dht_release_line();

    MQ2_SEL |= MQ2_BIT;
    MQ2_DIR &= (uint8_t)~MQ2_BIT;
    ADC12CTL0 = ADC12SHT0_4 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 = ADC12RES_2;

    HCSR04_TRIG_SEL &= (uint8_t)~HCSR04_TRIG_BIT;
    HCSR04_TRIG_DIR |= HCSR04_TRIG_BIT;
    HCSR04_TRIG_OUT &= (uint8_t)~HCSR04_TRIG_BIT;
    HCSR04_ECHO_SEL &= (uint8_t)~HCSR04_ECHO_BIT;
    HCSR04_ECHO_DIR &= (uint8_t)~HCSR04_ECHO_BIT;
    HCSR04_ECHO_REN &= (uint8_t)~HCSR04_ECHO_BIT;

    g_sensor_start_ms = board_millis();
}

void env_sensors_sample(EnvSensorSample *sample)
{
    sample->flags = 0u;
    if (dht_read(&sample->temperature_x10, &sample->humidity_x10) != 0u) {
        sample->flags |= ENV_SENSOR_DHT_OK;
    }

    sample->gas_adc = mq2_read_adc();
    sample->gas_level = mq2_level(sample->gas_adc);
    if ((uint32_t)(board_millis() - g_sensor_start_ms) >= MQ2_WARMUP_MS) {
        sample->flags |= ENV_SENSOR_MQ2_READY;
    }

    if (hcsr04_read_mm(&sample->distance_mm) != 0u) {
        sample->flags |= ENV_SENSOR_DISTANCE_OK;
    } else {
        sample->distance_mm = 0u;
    }
}
