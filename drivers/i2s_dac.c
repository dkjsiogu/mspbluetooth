#include "i2s_dac.h"

#include <msp430.h>

#include "board_pins.h"
#include "platform_config.h"

static uint8_t g_volume = PLAYER_DEFAULT_VOLUME;

static void i2s_set_bit(uint8_t bit_mask, uint8_t on)
{
    if (on != 0u) {
        I2S_OUT |= bit_mask;
    } else {
        I2S_OUT &= (uint8_t)~bit_mask;
    }
}

static void i2s_clock_bit(uint8_t bit)
{
    i2s_set_bit(I2S_DIN_BIT, bit);
    I2S_OUT |= I2S_BCK_BIT;
    __delay_cycles(1);
    I2S_OUT &= (uint8_t)~I2S_BCK_BIT;
    __delay_cycles(1);
}

static int16_t i2s_apply_volume(int16_t sample)
{
    int32_t scaled;

    scaled = (int32_t)sample * (int32_t)g_volume;
    scaled /= (int32_t)PLAYER_VOLUME_MAX;

    if (scaled > 32767L) {
        scaled = 32767L;
    } else if (scaled < -32768L) {
        scaled = -32768L;
    }

    return (int16_t)scaled;
}

static void i2s_write_channel(uint16_t sample, uint8_t right_channel)
{
    uint16_t mask;
    uint8_t padding_bits;

    i2s_set_bit(I2S_LRCK_BIT, right_channel);

    /* Standard I2S shifts the MSB one bit-clock after LRCK changes. */
    i2s_clock_bit(0u);

    mask = 0x8000u;
    while (mask != 0u) {
        i2s_clock_bit((uint8_t)((sample & mask) != 0u));
        mask >>= 1;
    }

    padding_bits = 15u;
    while (padding_bits > 0u) {
        i2s_clock_bit(0u);
        padding_bits--;
    }
}

void i2s_dac_init(void)
{
    I2S_SEL &= (uint8_t)~(I2S_BCK_BIT | I2S_LRCK_BIT | I2S_DIN_BIT);
    I2S_DIR |= I2S_BCK_BIT | I2S_LRCK_BIT | I2S_DIN_BIT;
    I2S_OUT &= (uint8_t)~(I2S_BCK_BIT | I2S_LRCK_BIT | I2S_DIN_BIT);
}

void i2s_dac_set_volume(uint8_t volume)
{
    if (volume > PLAYER_VOLUME_MAX) {
        volume = PLAYER_VOLUME_MAX;
    }
    g_volume = volume;
}

uint8_t i2s_dac_get_volume(void)
{
    return g_volume;
}

void i2s_dac_write_stereo(int16_t left, int16_t right)
{
    left = i2s_apply_volume(left);
    right = i2s_apply_volume(right);

    i2s_write_channel((uint16_t)left, 0u);
    i2s_write_channel((uint16_t)right, 1u);
}

void i2s_dac_write_silence(uint16_t frames)
{
    while (frames > 0u) {
        i2s_dac_write_stereo(0, 0);
        frames--;
    }
}
