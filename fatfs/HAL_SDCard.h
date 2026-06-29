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

/* SDCard_readFrame: reads size bytes into buffer; returns 1 on success. */
uint8_t SDCard_readFrame(uint8_t *buffer, uint16_t size);

/* SDCard_sendFrame: writes size bytes from buffer; returns 1 on success. */
uint8_t SDCard_sendFrame(const uint8_t *buffer, uint16_t size);

/* SDCard_setCSHigh: de-selects the TF card by driving chip select high. */
void SDCard_setCSHigh(void);

/* SDCard_setCSLow: selects the TF card by driving chip select low. */
void SDCard_setCSLow(void);

#endif
