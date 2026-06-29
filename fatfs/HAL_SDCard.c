#include "HAL_SDCard.h"

#include <msp430.h>

#include "board_pins.h"

static void sd_spi_delay(void)
{
    __delay_cycles(2);
}

static uint8_t sd_spi_transfer(uint8_t out_byte)
{
    uint8_t in_byte;
    uint8_t mask;

    in_byte = 0u;
    mask = 0x80u;

    while (mask != 0u) {
        if ((out_byte & mask) != 0u) {
            SD_SPI_OUT |= SD_SPI_MOSI_BIT;
        } else {
            SD_SPI_OUT &= (uint8_t)~SD_SPI_MOSI_BIT;
        }

        SD_SPI_OUT |= SD_SPI_SCK_BIT;
        sd_spi_delay();

        in_byte <<= 1;
        if ((SD_SPI_IN & SD_SPI_MISO_BIT) != 0u) {
            in_byte |= 1u;
        }

        SD_SPI_OUT &= (uint8_t)~SD_SPI_SCK_BIT;
        sd_spi_delay();

        mask >>= 1;
    }

    return in_byte;
}

unsigned char SDCard_init(void)
{
    SD_SPI_SEL &= (uint8_t)~(SD_SPI_SCK_BIT | SD_SPI_MOSI_BIT | SD_SPI_MISO_BIT);
    SD_SPI_DIR |= SD_SPI_SCK_BIT | SD_SPI_MOSI_BIT;
    SD_SPI_DIR &= (uint8_t)~SD_SPI_MISO_BIT;
    SD_SPI_REN |= SD_SPI_MISO_BIT;
    SD_SPI_OUT |= SD_SPI_MISO_BIT;
    SD_SPI_OUT &= (uint8_t)~SD_SPI_SCK_BIT;
    SD_SPI_OUT |= SD_SPI_MOSI_BIT;

    SD_CS_SEL &= (uint8_t)~SD_CS_BIT;
    SD_CS_DIR |= SD_CS_BIT;
    SD_CS_REN &= (uint8_t)~SD_CS_BIT;
    SD_CS_OUT |= SD_CS_BIT;

    return 0u;
}

void SDCard_fastMode(void)
{
    /* Bit-banged SPI has no divider to change; the function keeps FatFs API symmetry. */
}

uint8_t SDCard_readFrame(uint8_t *buffer, uint16_t size)
{
    while (size > 0u) {
        *buffer = sd_spi_transfer(0xFFu);
        buffer++;
        size--;
    }

    return 1u;
}

uint8_t SDCard_sendFrame(const uint8_t *buffer, uint16_t size)
{
    while (size > 0u) {
        (void)sd_spi_transfer(*buffer);
        buffer++;
        size--;
    }

    return 1u;
}

void SDCard_setCSHigh(void)
{
    SD_CS_OUT |= SD_CS_BIT;
}

void SDCard_setCSLow(void)
{
    SD_CS_OUT &= (uint8_t)~SD_CS_BIT;
}

