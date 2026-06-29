/*
 * HAL_SDCard.h
 * Hardware adapter used by FatFs mmc.c. It hides the bit-banged TF-card SPI
 * pins, chip-select control, and byte-frame transfer primitives from the
 * generic disk I/O layer.
 */
#ifndef HAL_SDCARD_H
#define HAL_SDCARD_H

#include <stdint.h>

/* SDCard_init: configures TF-card software SPI GPIO and releases chip select. */
unsigned char SDCard_init(void);

/* SDCard_fastMode: switches to high-speed mode when supported by the adapter. */
void SDCard_fastMode(void);

/*
 * SDCard_readFrame: reads bytes from the TF-card SPI bus.
 * buffer: destination buffer for received bytes.
 * size: number of bytes to read.
 */
uint8_t SDCard_readFrame(uint8_t *buffer, uint16_t size);

/*
 * SDCard_sendFrame: writes bytes to the TF-card SPI bus.
 * buffer: source buffer containing bytes to transmit.
 * size: number of bytes to write.
 */
uint8_t SDCard_sendFrame(const uint8_t *buffer, uint16_t size);

/* SDCard_setCSHigh: de-selects the TF card by driving chip select high. */
void SDCard_setCSHigh(void);

/* SDCard_setCSLow: selects the TF card by driving chip select low. */
void SDCard_setCSLow(void);

#endif
