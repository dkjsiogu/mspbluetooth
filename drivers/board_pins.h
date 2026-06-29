#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <msp430.h>

/* Status LED: MSP430F5529 LaunchPad/Pocket Kit LED on P1.0. */
#define STATUS_LED_DIR                  P1DIR
#define STATUS_LED_OUT                  P1OUT
#define STATUS_LED_BIT                  BIT0

/* TF card SPI, kept on the course-design pins. SPI is bit-banged. */
#define SD_CS_SEL                       P4SEL
#define SD_CS_DIR                       P4DIR
#define SD_CS_OUT                       P4OUT
#define SD_CS_REN                       P4REN
#define SD_CS_BIT                       BIT0

#define SD_SPI_SEL                      P3SEL
#define SD_SPI_DIR                      P3DIR
#define SD_SPI_OUT                      P3OUT
#define SD_SPI_IN                       P3IN
#define SD_SPI_REN                      P3REN
#define SD_SPI_SCK_BIT                  BIT1
#define SD_SPI_MOSI_BIT                 BIT2
#define SD_SPI_MISO_BIT                 BIT3

/* PCM5102A I2S input pins. */
#define I2S_SEL                         P4SEL
#define I2S_DIR                         P4DIR
#define I2S_OUT                         P4OUT
#define I2S_BCK_BIT                     BIT1
#define I2S_LRCK_BIT                    BIT2
#define I2S_DIN_BIT                     BIT3

/* EC11 rotary encoder with push button. Active level is low. */
#define ENC_SEL                         P2SEL
#define ENC_DIR                         P2DIR
#define ENC_OUT                         P2OUT
#define ENC_IN                          P2IN
#define ENC_REN                         P2REN
#define ENC_A_BIT                       BIT1
#define ENC_B_BIT                       BIT2
#define ENC_SW_BIT                      BIT3

/* Bluetooth UART alternatives. */
#define BT_UCA1_SEL                     P4SEL
#define BT_UCA1_DIR                     P4DIR
#define BT_UCA1_TX_BIT                  BIT4
#define BT_UCA1_RX_BIT                  BIT5

#define BT_UCA0_SEL                     P3SEL
#define BT_UCA0_DIR                     P3DIR
#define BT_UCA0_TX_BIT                  BIT3
#define BT_UCA0_RX_BIT                  BIT4

#endif

