/*
 * main.c
 * MSP430F5529 多点温度/环境监测系统入口。文件只保留启动顺序和前台
 * 调度循环，具体采集、显示、蓝牙、Flash 和报警逻辑放在 env_monitor。
 */
#include <msp430.h>

#include "application/env_monitor.h"
#include "drivers/bluetooth_uart.h"
#include "drivers/board.h"
#include "drivers/encoder.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    board_clock_init();
    board_gpio_init();
    board_tick_init();
    bluetooth_uart_init();
    encoder_init();

    __enable_interrupt();

    env_monitor_init();

    while (1) {
        env_monitor_service();
        board_idle();
    }
}
