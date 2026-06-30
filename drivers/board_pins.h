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

/* TF card SPI, kept on the course-design pins. SPI is bit-banged. */
/* SD_CS_SEL: function-select register for TF-card chip select. */
#define SD_CS_SEL                       P4SEL
/* SD_CS_DIR: direction register for TF-card chip select. */
#define SD_CS_DIR                       P4DIR
/* SD_CS_OUT: output latch register for TF-card chip select. */
#define SD_CS_OUT                       P4OUT
/* SD_CS_REN: pull resistor register for TF-card chip select. */
#define SD_CS_REN                       P4REN
/* SD_CS_BIT: TF-card chip-select bit mask, active low. */
#define SD_CS_BIT                       BIT0

/* SD_SPI_SEL: function-select register for software SPI pins. */
#define SD_SPI_SEL                      P3SEL
/* SD_SPI_DIR: direction register for software SPI pins. */
#define SD_SPI_DIR                      P3DIR
/* SD_SPI_OUT: output latch register for software SPI pins. */
#define SD_SPI_OUT                      P3OUT
/* SD_SPI_IN: input register for software SPI pins. */
#define SD_SPI_IN                       P3IN
/* SD_SPI_REN: pull resistor register for software SPI pins. */
#define SD_SPI_REN                      P3REN
/* SD_SPI_SCK_BIT: TF-card SPI clock bit mask. */
#define SD_SPI_SCK_BIT                  BIT1
/* SD_SPI_MOSI_BIT: TF-card SPI MOSI bit mask. */
#define SD_SPI_MOSI_BIT                 BIT2
/* SD_SPI_MISO_BIT: TF-card SPI MISO bit mask. */
#define SD_SPI_MISO_BIT                 BIT3

/* PCM5102A I2S input pins. */
/* I2S_SEL: function-select register for software I2S pins. */
#define I2S_SEL                         P4SEL
/* I2S_DIR: direction register for software I2S pins. */
#define I2S_DIR                         P4DIR
/* I2S_OUT: output latch register for software I2S pins. */
#define I2S_OUT                         P4OUT
/* I2S_BCK_BIT: PCM5102A bit-clock output bit mask. */
#define I2S_BCK_BIT                     BIT1
/* I2S_LRCK_BIT: PCM5102A left/right clock output bit mask. */
#define I2S_LRCK_BIT                    BIT2
/* I2S_DIN_BIT: PCM5102A serial data output bit mask. */
#define I2S_DIN_BIT                     BIT3

/* EC11 rotary encoder with push button. Active level is low. */
/* ENC_SEL: function-select register for EC11 pins. */
#define ENC_SEL                         P2SEL
/* ENC_DIR: direction register for EC11 pins. */
#define ENC_DIR                         P2DIR
/* ENC_OUT: output latch register used for EC11 pull-ups. */
#define ENC_OUT                         P2OUT
/* ENC_IN: input register used to sample EC11 states. */
#define ENC_IN                          P2IN
/* ENC_REN: pull resistor register for EC11 pins. */
#define ENC_REN                         P2REN
/* ENC_A_BIT: EC11 phase-A bit mask. */
#define ENC_A_BIT                       BIT1
/* ENC_B_BIT: EC11 phase-B bit mask. */
#define ENC_B_BIT                       BIT2
/* ENC_SW_BIT: EC11 push-switch bit mask, active low. */
#define ENC_SW_BIT                      BIT3

/* Local board buttons; S3/P2.3 is intentionally reserved for EC11 switch. */
/* LOCAL_BTN1_SEL: function-select register for S1 on P1.2. */
#define LOCAL_BTN1_SEL                  P1SEL
/* LOCAL_BTN1_DIR: direction register for S1 on P1.2. */
#define LOCAL_BTN1_DIR                  P1DIR
/* LOCAL_BTN1_OUT: output latch register used to enable S1 pull-up. */
#define LOCAL_BTN1_OUT                  P1OUT
/* LOCAL_BTN1_IN: input register used to sample S1. */
#define LOCAL_BTN1_IN                   P1IN
/* LOCAL_BTN1_REN: pull resistor register for S1. */
#define LOCAL_BTN1_REN                  P1REN
/* LOCAL_BTN1_BIT: S1 bit mask, active low. */
#define LOCAL_BTN1_BIT                  BIT2

/* LOCAL_BTN2_SEL: function-select register for S2 on P1.3. */
#define LOCAL_BTN2_SEL                  P1SEL
/* LOCAL_BTN2_DIR: direction register for S2 on P1.3. */
#define LOCAL_BTN2_DIR                  P1DIR
/* LOCAL_BTN2_OUT: output latch register used to enable S2 pull-up. */
#define LOCAL_BTN2_OUT                  P1OUT
/* LOCAL_BTN2_IN: input register used to sample S2. */
#define LOCAL_BTN2_IN                   P1IN
/* LOCAL_BTN2_REN: pull resistor register for S2. */
#define LOCAL_BTN2_REN                  P1REN
/* LOCAL_BTN2_BIT: S2 bit mask, active low. */
#define LOCAL_BTN2_BIT                  BIT3

/* LOCAL_BTN4_SEL: function-select register for S4 on P2.6. */
#define LOCAL_BTN4_SEL                  P2SEL
/* LOCAL_BTN4_DIR: direction register for S4 on P2.6. */
#define LOCAL_BTN4_DIR                  P2DIR
/* LOCAL_BTN4_OUT: output latch register used to enable S4 pull-up. */
#define LOCAL_BTN4_OUT                  P2OUT
/* LOCAL_BTN4_IN: input register used to sample S4. */
#define LOCAL_BTN4_IN                   P2IN
/* LOCAL_BTN4_REN: pull resistor register for S4. */
#define LOCAL_BTN4_REN                  P2REN
/* LOCAL_BTN4_BIT: S4 bit mask, active low. */
#define LOCAL_BTN4_BIT                  BIT6

/* Bluetooth UART alternatives. */
/* BT_UCA1_SEL: function-select register for UCA1 Bluetooth pins. */
#define BT_UCA1_SEL                     P4SEL
/* BT_UCA1_DIR: direction register for UCA1 Bluetooth pins. */
#define BT_UCA1_DIR                     P4DIR
/* BT_UCA1_TX_BIT: UCA1 TXD bit mask, connect to HC-05 RXD. */
#define BT_UCA1_TX_BIT                  BIT4
/* BT_UCA1_RX_BIT: UCA1 RXD bit mask, connect to HC-05 TXD. */
#define BT_UCA1_RX_BIT                  BIT5

/* BT_UCA0_SEL: function-select register for alternate UCA0 Bluetooth pins. */
#define BT_UCA0_SEL                     P3SEL
/* BT_UCA0_DIR: direction register for alternate UCA0 Bluetooth pins. */
#define BT_UCA0_DIR                     P3DIR
/* BT_UCA0_TX_BIT: alternate UCA0 TXD bit mask, conflicts with TF MISO. */
#define BT_UCA0_TX_BIT                  BIT3
/* BT_UCA0_RX_BIT: alternate UCA0 RXD bit mask. */
#define BT_UCA0_RX_BIT                  BIT4

/*
 * Optional e-paper panel on P6.x. This is intentionally disabled in the default
 * firmware build and exists for a later hardware spin where the audio player
 * core remains unchanged.
 */
/* EPAPER_SEL: function-select register for optional e-paper GPIO pins. */
#define EPAPER_SEL                      P6SEL
/* EPAPER_DIR: direction register for optional e-paper GPIO pins. */
#define EPAPER_DIR                      P6DIR
/* EPAPER_OUT: output latch register for optional e-paper GPIO pins. */
#define EPAPER_OUT                      P6OUT
/* EPAPER_IN: input register for optional e-paper GPIO pins. */
#define EPAPER_IN                       P6IN
/* EPAPER_REN: pull resistor register for optional e-paper GPIO pins. */
#define EPAPER_REN                      P6REN
/* EPAPER_SCK_BIT: optional e-paper software SPI clock on P6.0. */
#define EPAPER_SCK_BIT                  BIT0
/* EPAPER_MOSI_BIT: optional e-paper software SPI MOSI on P6.1. */
#define EPAPER_MOSI_BIT                 BIT1
/* EPAPER_CS_BIT: optional e-paper chip select on P6.2, active low. */
#define EPAPER_CS_BIT                   BIT2
/* EPAPER_DC_BIT: optional e-paper data/command select on P6.3. */
#define EPAPER_DC_BIT                   BIT3
/* EPAPER_RST_BIT: optional e-paper reset output on P6.4, active low. */
#define EPAPER_RST_BIT                  BIT4
/* EPAPER_BUSY_BIT: optional e-paper busy input on P6.5, high while busy. */
#define EPAPER_BUSY_BIT                 BIT5

#endif
