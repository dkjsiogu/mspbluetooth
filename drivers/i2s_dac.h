/*
 * i2s_dac.h
 * Software I2S transmitter for the PCM5102A DAC. It writes 16-bit stereo
 * samples through GPIO pins and applies a small integer volume scale before
 * sending data to the DAC.
 */
#ifndef I2S_DAC_H
#define I2S_DAC_H

#include <stdint.h>

/* i2s_dac_init: configures BCK/LRCK/DIN pins and clears the bus idle state. */
void i2s_dac_init(void);

/* i2s_dac_set_volume: sets software gain; volume is clamped to PLAYER_VOLUME_MAX. */
void i2s_dac_set_volume(uint8_t volume);

/* i2s_dac_get_volume: returns the current software volume step. */
uint8_t i2s_dac_get_volume(void);

/* i2s_dac_write_stereo: sends one stereo frame; left/right are signed PCM samples. */
void i2s_dac_write_stereo(int16_t left, int16_t right);

/* i2s_dac_write_silence: sends frames zero-amplitude frames to quiet the DAC. */
void i2s_dac_write_silence(uint16_t frames);

/* i2s_dac_write_test_tone: emits frames square-wave frames using amplitude. */
void i2s_dac_write_test_tone(uint16_t frames, int16_t amplitude);

#endif
