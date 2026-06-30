/*
 * board_pins.h
 * 多点温度/环境监测系统的物理引脚表。所有底层驱动通过本文件访问
 * MSP430 端口寄存器，便于集中检查 DHT11、MQ-2、超声波、OLED、蓝牙、
 * 蜂鸣器和编码器之间的引脚冲突。
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <msp430.h>

/* STATUS_LED_DIR: 状态指示灯方向寄存器；P1.0 已留给 DHT11，状态灯改用 P4.7。 */
#define STATUS_LED_DIR                  P4DIR
/* STATUS_LED_OUT: 状态指示灯输出锁存寄存器。 */
#define STATUS_LED_OUT                  P4OUT
/* STATUS_LED_BIT: 状态指示灯位掩码。 */
#define STATUS_LED_BIT                  BIT7

/* DHT11_SEL: DHT11 DATA=P1.0 的功能选择寄存器。 */
#define DHT11_SEL                       P1SEL
/* DHT11_DIR: DHT11 DATA=P1.0 的方向寄存器。 */
#define DHT11_DIR                       P1DIR
/* DHT11_OUT: DHT11 DATA=P1.0 的输出锁存寄存器。 */
#define DHT11_OUT                       P1OUT
/* DHT11_IN: DHT11 DATA=P1.0 的输入寄存器。 */
#define DHT11_IN                        P1IN
/* DHT11_REN: DHT11 DATA=P1.0 的上下拉电阻寄存器。 */
#define DHT11_REN                       P1REN
/* DHT11_BIT: DHT11 DATA=P1.0 的位掩码。 */
#define DHT11_BIT                       BIT0

/* MQ2_SEL: MQ-2 AO=P6.0 的功能选择寄存器。 */
#define MQ2_SEL                         P6SEL
/* MQ2_DIR: MQ-2 AO=P6.0 的方向寄存器。 */
#define MQ2_DIR                         P6DIR
/* MQ2_BIT: MQ-2 AO=P6.0 的位掩码，对应 ADC12_A0。 */
#define MQ2_BIT                         BIT0

/* HCSR04_TRIG_SEL: HC-SR04 Trig=P1.2 的功能选择寄存器。 */
#define HCSR04_TRIG_SEL                 P1SEL
/* HCSR04_TRIG_DIR: HC-SR04 Trig=P1.2 的方向寄存器。 */
#define HCSR04_TRIG_DIR                 P1DIR
/* HCSR04_TRIG_OUT: HC-SR04 Trig=P1.2 的输出锁存寄存器。 */
#define HCSR04_TRIG_OUT                 P1OUT
/* HCSR04_TRIG_BIT: HC-SR04 Trig=P1.2 的位掩码。 */
#define HCSR04_TRIG_BIT                 BIT2
/* HCSR04_ECHO_SEL: HC-SR04 Echo=P1.3 的功能选择寄存器。 */
#define HCSR04_ECHO_SEL                 P1SEL
/* HCSR04_ECHO_DIR: HC-SR04 Echo=P1.3 的方向寄存器。 */
#define HCSR04_ECHO_DIR                 P1DIR
/* HCSR04_ECHO_IN: HC-SR04 Echo=P1.3 的输入寄存器。 */
#define HCSR04_ECHO_IN                  P1IN
/* HCSR04_ECHO_REN: HC-SR04 Echo=P1.3 的上下拉电阻寄存器。 */
#define HCSR04_ECHO_REN                 P1REN
/* HCSR04_ECHO_OUT: HC-SR04 Echo=P1.3 的上拉/下拉选择寄存器。 */
#define HCSR04_ECHO_OUT                 P1OUT
/* HCSR04_ECHO_BIT: HC-SR04 Echo=P1.3 的位掩码。 */
#define HCSR04_ECHO_BIT                 BIT3

/* OLED_SEL: OLED 软件 I2C SCL/SDA 的功能选择寄存器。 */
#define OLED_SEL                        P3SEL
/* OLED_DIR: OLED 软件 I2C SCL/SDA 的方向寄存器。 */
#define OLED_DIR                        P3DIR
/* OLED_OUT: OLED 软件 I2C SCL/SDA 的输出锁存寄存器。 */
#define OLED_OUT                        P3OUT
/* OLED_REN: OLED 软件 I2C SCL/SDA 的上下拉电阻寄存器。 */
#define OLED_REN                        P3REN
/* OLED_SDA_BIT: OLED SDA=P3.0 的位掩码。 */
#define OLED_SDA_BIT                    BIT0
/* OLED_SCL_BIT: OLED SCL=P3.1 的位掩码。 */
#define OLED_SCL_BIT                    BIT1

/* BUZZER_SEL: 蜂鸣器控制脚 P2.0 的功能选择寄存器。 */
#define BUZZER_SEL                      P2SEL
/* BUZZER_DIR: 蜂鸣器控制脚 P2.0 的方向寄存器。 */
#define BUZZER_DIR                      P2DIR
/* BUZZER_OUT: 蜂鸣器控制脚 P2.0 的输出锁存寄存器。 */
#define BUZZER_OUT                      P2OUT
/* BUZZER_BIT: 蜂鸣器控制脚 P2.0 的位掩码。 */
#define BUZZER_BIT                      BIT0

/* ENC_SEL: EC11 编码器引脚功能选择寄存器。 */
#define ENC_SEL                         P2SEL
/* ENC_DIR: EC11 编码器引脚方向寄存器。 */
#define ENC_DIR                         P2DIR
/* ENC_OUT: EC11 上拉输出锁存寄存器。 */
#define ENC_OUT                         P2OUT
/* ENC_IN: EC11 相位和按键输入寄存器。 */
#define ENC_IN                          P2IN
/* ENC_REN: EC11 上拉电阻使能寄存器。 */
#define ENC_REN                         P2REN
/* ENC_A_BIT: EC11 A 相位 P2.1 位掩码。 */
#define ENC_A_BIT                       BIT1
/* ENC_B_BIT: EC11 B 相位 P2.2 位掩码。 */
#define ENC_B_BIT                       BIT2
/* ENC_SW_BIT: EC11 按键 P2.3 位掩码，低电平按下。 */
#define ENC_SW_BIT                      BIT3

/* BT_UCA1_SEL: HC-05 使用 UCA1 时的功能选择寄存器。 */
#define BT_UCA1_SEL                     P4SEL
/* BT_UCA1_DIR: HC-05 使用 UCA1 时的方向寄存器。 */
#define BT_UCA1_DIR                     P4DIR
/* BT_UCA1_TX_BIT: P4.4/UCA1TXD，连接 HC-05 RXD。 */
#define BT_UCA1_TX_BIT                  BIT4
/* BT_UCA1_RX_BIT: P4.5/UCA1RXD，连接 HC-05 TXD。 */
#define BT_UCA1_RX_BIT                  BIT5

/* BT_UCA0_SEL: 备用 UCA0 蓝牙引脚功能选择寄存器。 */
#define BT_UCA0_SEL                     P3SEL
/* BT_UCA0_DIR: 备用 UCA0 蓝牙引脚方向寄存器。 */
#define BT_UCA0_DIR                     P3DIR
/* BT_UCA0_TX_BIT: P3.3/UCA0TXD 备用发送位掩码。 */
#define BT_UCA0_TX_BIT                  BIT3
/* BT_UCA0_RX_BIT: P3.4/UCA0RXD 备用接收位掩码。 */
#define BT_UCA0_RX_BIT                  BIT4

#endif
