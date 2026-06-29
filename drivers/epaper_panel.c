/*
 * epaper_panel.c
 * Optional synchronous SPI e-paper renderer for the three-line player display.
 * It targets common SSD1680/UC8151-style 296x128 black/white modules. The code
 * is compiled in the normal build but becomes no-op unless
 * PLAYER_ENABLE_EPAPER_PANEL is set to 1 in platform_config.h.
 */
#include "epaper_panel.h"

#include "board.h"
#include "board_pins.h"
#include "platform_config.h"

/* EPAPER_CMD_DRIVER_OUTPUT: controller command for scan-line count. */
#define EPAPER_CMD_DRIVER_OUTPUT        0x01u
/* EPAPER_CMD_DEEP_SLEEP: controller command for deep-sleep entry. */
#define EPAPER_CMD_DEEP_SLEEP           0x10u
/* EPAPER_CMD_DATA_ENTRY: controller command for RAM address increment mode. */
#define EPAPER_CMD_DATA_ENTRY           0x11u
/* EPAPER_CMD_SW_RESET: controller command for software reset. */
#define EPAPER_CMD_SW_RESET             0x12u
/* EPAPER_CMD_WRITE_BW_RAM: controller command for black/white image RAM. */
#define EPAPER_CMD_WRITE_BW_RAM         0x24u
/* EPAPER_CMD_DISPLAY_UPDATE: controller command for update-control byte. */
#define EPAPER_CMD_DISPLAY_UPDATE       0x22u
/* EPAPER_CMD_MASTER_ACTIVATE: controller command that starts refresh. */
#define EPAPER_CMD_MASTER_ACTIVATE      0x20u
/* EPAPER_CMD_BORDER: controller command for border waveform behavior. */
#define EPAPER_CMD_BORDER               0x3Cu
/* EPAPER_CMD_SET_RAM_X: controller command for X byte address window. */
#define EPAPER_CMD_SET_RAM_X            0x44u
/* EPAPER_CMD_SET_RAM_Y: controller command for Y pixel address window. */
#define EPAPER_CMD_SET_RAM_Y            0x45u
/* EPAPER_CMD_SET_RAM_X_COUNTER: controller command for X start counter. */
#define EPAPER_CMD_SET_RAM_X_COUNTER    0x4Eu
/* EPAPER_CMD_SET_RAM_Y_COUNTER: controller command for Y start counter. */
#define EPAPER_CMD_SET_RAM_Y_COUNTER    0x4Fu
/* EPAPER_CMD_POWER_ON: controller command that turns charge pumps on. */
#define EPAPER_CMD_POWER_ON             0x04u
/* EPAPER_CMD_POWER_OFF: controller command that turns charge pumps off. */
#define EPAPER_CMD_POWER_OFF            0x02u

/* EPAPER_TEXT_SCALE: integer scale applied to the 5x7 ASCII glyphs. */
#define EPAPER_TEXT_SCALE               2u
/* EPAPER_GLYPH_WIDTH: native glyph bitmap width in pixels. */
#define EPAPER_GLYPH_WIDTH              5u
/* EPAPER_GLYPH_HEIGHT: native glyph bitmap height in pixels. */
#define EPAPER_GLYPH_HEIGHT             7u
/* EPAPER_CELL_WIDTH: scaled character cell width including one blank column. */
#define EPAPER_CELL_WIDTH               ((EPAPER_GLYPH_WIDTH + 1u) * EPAPER_TEXT_SCALE)
/* EPAPER_LINE1_Y: top pixel for display line 1. */
#define EPAPER_LINE1_Y                  10u
/* EPAPER_LINE2_Y: top pixel for display line 2. */
#define EPAPER_LINE2_Y                  52u
/* EPAPER_LINE3_Y: top pixel for display line 3. */
#define EPAPER_LINE3_Y                  94u
/* EPAPER_TEXT_X: left margin for all text lines. */
#define EPAPER_TEXT_X                   4u
/* EPAPER_BUSY_TIMEOUT_MS: maximum wait for BUSY release during refresh. */
#define EPAPER_BUSY_TIMEOUT_MS          8000u

#if PLAYER_ENABLE_EPAPER_PANEL != 0u

/* g_font5x7: printable ASCII 0x20..0x7F, columns are LSB-at-top. */
static const uint8_t g_font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
    {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
    {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00},
    {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18},
    {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00},
    {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C},
    {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C},
    {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00},
    {0x10,0x08,0x08,0x10,0x08}, {0x00,0x06,0x09,0x09,0x06}
};

/* epaper_select: asserts panel chip select for one SPI transfer. */
static void epaper_select(void)
{
    EPAPER_OUT &= (uint8_t)~EPAPER_CS_BIT;
}

/* epaper_deselect: releases panel chip select after one SPI transfer. */
static void epaper_deselect(void)
{
    EPAPER_OUT |= EPAPER_CS_BIT;
}

/* epaper_write_byte: bit-bangs one MSB-first SPI byte to the panel. */
static void epaper_write_byte(uint8_t value)
{
    uint8_t bit;

    for (bit = 0u; bit < 8u; bit++) {
        if ((value & 0x80u) != 0u) {
            EPAPER_OUT |= EPAPER_MOSI_BIT;
        } else {
            EPAPER_OUT &= (uint8_t)~EPAPER_MOSI_BIT;
        }
        EPAPER_OUT |= EPAPER_SCK_BIT;
        EPAPER_OUT &= (uint8_t)~EPAPER_SCK_BIT;
        value <<= 1;
    }
}

/* epaper_command: writes one controller command byte. */
static void epaper_command(uint8_t command)
{
    EPAPER_OUT &= (uint8_t)~EPAPER_DC_BIT;
    epaper_select();
    epaper_write_byte(command);
    epaper_deselect();
}

/* epaper_data: writes one controller data byte. */
static void epaper_data(uint8_t data)
{
    EPAPER_OUT |= EPAPER_DC_BIT;
    epaper_select();
    epaper_write_byte(data);
    epaper_deselect();
}

/* epaper_wait_idle: waits until BUSY is released, with a bounded timeout. */
static void epaper_wait_idle(void)
{
    uint16_t elapsed_ms;

    elapsed_ms = 0u;
    while (((EPAPER_IN & EPAPER_BUSY_BIT) != 0u) && (elapsed_ms < EPAPER_BUSY_TIMEOUT_MS)) {
        board_delay_ms(1u);
        elapsed_ms++;
    }
}

/* epaper_reset: performs the panel hardware reset pulse. */
static void epaper_reset(void)
{
    EPAPER_OUT |= EPAPER_RST_BIT;
    board_delay_ms(20u);
    EPAPER_OUT &= (uint8_t)~EPAPER_RST_BIT;
    board_delay_ms(10u);
    EPAPER_OUT |= EPAPER_RST_BIT;
    board_delay_ms(20u);
}

/* epaper_set_window: selects the full 296x128 RAM drawing window. */
static void epaper_set_window(void)
{
    epaper_command(EPAPER_CMD_SET_RAM_X);
    epaper_data(0u);
    epaper_data((uint8_t)(EPAPER_ROW_BYTES - 1u));

    epaper_command(EPAPER_CMD_SET_RAM_Y);
    epaper_data(0u);
    epaper_data(0u);
    epaper_data((uint8_t)(EPAPER_HEIGHT_PIXELS - 1u));
    epaper_data(0u);

    epaper_command(EPAPER_CMD_SET_RAM_X_COUNTER);
    epaper_data(0u);
    epaper_command(EPAPER_CMD_SET_RAM_Y_COUNTER);
    epaper_data(0u);
    epaper_data(0u);
}

/* epaper_glyph_pixel: returns nonzero when a glyph pixel should be black. */
static uint8_t epaper_glyph_pixel(char raw_char, uint8_t glyph_x, uint8_t glyph_y)
{
    uint8_t ascii;
    uint8_t column_bits;

    if ((raw_char < ' ') || (raw_char > '~')) {
        raw_char = '?';
    }
    ascii = (uint8_t)raw_char;
    if ((glyph_x >= EPAPER_GLYPH_WIDTH) || (glyph_y >= EPAPER_GLYPH_HEIGHT)) {
        return 0u;
    }

    column_bits = g_font5x7[ascii - (uint8_t)' '][glyph_x];
    return (uint8_t)((column_bits >> glyph_y) & 0x01u);
}

/* epaper_line_pixel: returns nonzero when line text covers x/y. */
static uint8_t epaper_line_pixel(const char *line, uint16_t x, uint16_t y, uint16_t line_y)
{
    uint16_t relative_x;
    uint16_t relative_y;
    uint8_t char_index;
    uint8_t cell_x;
    uint8_t glyph_x;
    uint8_t glyph_y;

    if ((x < EPAPER_TEXT_X) || (y < line_y)) {
        return 0u;
    }

    relative_x = (uint16_t)(x - EPAPER_TEXT_X);
    relative_y = (uint16_t)(y - line_y);
    if (relative_y >= (EPAPER_GLYPH_HEIGHT * EPAPER_TEXT_SCALE)) {
        return 0u;
    }

    char_index = (uint8_t)(relative_x / EPAPER_CELL_WIDTH);
    if ((char_index >= DISPLAY_MODEL_LINE_CHARS) || (line[char_index] == '\0')) {
        return 0u;
    }

    cell_x = (uint8_t)(relative_x % EPAPER_CELL_WIDTH);
    glyph_x = (uint8_t)(cell_x / EPAPER_TEXT_SCALE);
    glyph_y = (uint8_t)(relative_y / EPAPER_TEXT_SCALE);
    return epaper_glyph_pixel(line[char_index], glyph_x, glyph_y);
}

/* epaper_frame_pixel: returns nonzero when any display line covers x/y. */
static uint8_t epaper_frame_pixel(const DisplayFrame *frame, uint16_t x, uint16_t y)
{
    if (epaper_line_pixel(frame->line1, x, y, EPAPER_LINE1_Y) != 0u) {
        return 1u;
    }
    if (epaper_line_pixel(frame->line2, x, y, EPAPER_LINE2_Y) != 0u) {
        return 1u;
    }
    return epaper_line_pixel(frame->line3, x, y, EPAPER_LINE3_Y);
}

/* epaper_frame_byte: builds one horizontal byte, where 0 bits become black. */
static uint8_t epaper_frame_byte(const DisplayFrame *frame, uint16_t byte_x, uint16_t y)
{
    uint8_t bit;
    uint8_t value;
    uint16_t x;

    value = 0xFFu;
    for (bit = 0u; bit < 8u; bit++) {
        x = (uint16_t)((byte_x * 8u) + bit);
        if (epaper_frame_pixel(frame, x, y) != 0u) {
            value &= (uint8_t)~(uint8_t)(0x80u >> bit);
        }
    }
    return value;
}

void epaper_panel_init(void)
{
    EPAPER_SEL &= (uint8_t)~(EPAPER_SCK_BIT | EPAPER_MOSI_BIT | EPAPER_CS_BIT |
                             EPAPER_DC_BIT | EPAPER_RST_BIT | EPAPER_BUSY_BIT);
    EPAPER_DIR |= (EPAPER_SCK_BIT | EPAPER_MOSI_BIT | EPAPER_CS_BIT |
                   EPAPER_DC_BIT | EPAPER_RST_BIT);
    EPAPER_DIR &= (uint8_t)~EPAPER_BUSY_BIT;
    EPAPER_REN &= (uint8_t)~EPAPER_BUSY_BIT;
    EPAPER_OUT |= EPAPER_CS_BIT | EPAPER_RST_BIT;
    EPAPER_OUT &= (uint8_t)~(EPAPER_SCK_BIT | EPAPER_MOSI_BIT | EPAPER_DC_BIT);

    epaper_reset();
    epaper_command(EPAPER_CMD_SW_RESET);
    epaper_wait_idle();

    epaper_command(EPAPER_CMD_DRIVER_OUTPUT);
    epaper_data((uint8_t)(EPAPER_HEIGHT_PIXELS - 1u));
    epaper_data(0u);
    epaper_data(0u);

    epaper_command(EPAPER_CMD_DATA_ENTRY);
    epaper_data(0x03u);

    epaper_command(EPAPER_CMD_BORDER);
    epaper_data(0x05u);
    epaper_set_window();
}

uint8_t epaper_panel_enabled(void)
{
    return 1u;
}

void epaper_panel_show_frame(const DisplayFrame *frame)
{
    uint16_t y;
    uint16_t byte_x;

    epaper_command(EPAPER_CMD_POWER_ON);
    epaper_wait_idle();
    epaper_set_window();
    epaper_command(EPAPER_CMD_WRITE_BW_RAM);

    for (y = 0u; y < EPAPER_HEIGHT_PIXELS; y++) {
        for (byte_x = 0u; byte_x < EPAPER_ROW_BYTES; byte_x++) {
            epaper_data(epaper_frame_byte(frame, byte_x, y));
        }
    }

    epaper_command(EPAPER_CMD_DISPLAY_UPDATE);
    epaper_data(0xF7u);
    epaper_command(EPAPER_CMD_MASTER_ACTIVATE);
    epaper_wait_idle();
    epaper_command(EPAPER_CMD_POWER_OFF);
    epaper_wait_idle();
}

#else

void epaper_panel_init(void)
{
}

uint8_t epaper_panel_enabled(void)
{
    return 0u;
}

void epaper_panel_show_frame(const DisplayFrame *frame)
{
    (void)frame;
}

#endif
