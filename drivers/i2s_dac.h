#ifndef I2S_DAC_H
#define I2S_DAC_H

#include <stdint.h>

void i2s_dac_init(void);
void i2s_dac_set_volume(uint8_t volume);
uint8_t i2s_dac_get_volume(void);
void i2s_dac_write_stereo(int16_t left, int16_t right);
void i2s_dac_write_silence(uint16_t frames);

#endif

