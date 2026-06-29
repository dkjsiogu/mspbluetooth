#ifndef HAL_SDCARD_H
#define HAL_SDCARD_H

#include <stdint.h>

unsigned char SDCard_init(void);
void SDCard_fastMode(void);
uint8_t SDCard_readFrame(uint8_t *buffer, uint16_t size);
uint8_t SDCard_sendFrame(const uint8_t *buffer, uint16_t size);
void SDCard_setCSHigh(void);
void SDCard_setCSLow(void);

#endif

