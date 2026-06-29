/*
 * board_pins.h
 * Physical pin map for the player hardware. All low-level drivers include this
 * file instead of hard-coding MSP430 port registers, making pin conflicts easy
 * to audit before wiring the board.
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <msp430.h>

/* Status LED: MSP430F5529 LaunchPad/Pocket Kit LED on P1.0. */
/* STATUS_LED_DIR: direction register for the heartbeat/status LED. */
#define STATUS_LED_DIR                  P1DIR
/* STATUS_LED_OUT: output latch register for the heartbeat/status LED. */
#define STATUS_LED_OUT                  P1OUT
/* STATUS_LED_BIT: bit mask of the heartbeat/status LED. */
#define STATUS_LED_BIT                  BIT0

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

#endif
