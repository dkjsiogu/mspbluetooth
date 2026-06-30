/*
 * oled_ssd1306.c
 * SSD1306 OLED 的软件 I2C 与精简 5x7 字库实现。字库只覆盖本项目
 * 页面需要的数字、英文字母和常用符号，降低 MSP430 Flash 占用。
 */
#include "oled_ssd1306.h"

#include <stdint.h>

#include "board.h"
#include "board_pins.h"

/* OLED_ADDR: SSD1306 常用 7 位地址 0x3C 转成写地址。 */
#define OLED_ADDR                       0x78u

/* i2c_delay: 软件 I2C 半周期延时。 */
static void i2c_delay(void)
{
    board_delay_us(4u);
}

/* scl_high: 释放 SCL 为高电平。 */
static void scl_high(void)
{
    OLED_OUT |= OLED_SCL_BIT;
    i2c_delay();
}

/* scl_low: 拉低 SCL。 */
static void scl_low(void)
{
    OLED_OUT &= (uint8_t)~OLED_SCL_BIT;
    i2c_delay();
}

/* sda_high: 释放 SDA 为高电平。 */
static void sda_high(void)
{
    OLED_OUT |= OLED_SDA_BIT;
    i2c_delay();
}

/* sda_low: 拉低 SDA。 */
static void sda_low(void)
{
    OLED_OUT &= (uint8_t)~OLED_SDA_BIT;
    i2c_delay();
}

/* i2c_start: 发送软件 I2C 起始条件。 */
static void i2c_start(void)
{
    sda_high();
    scl_high();
    sda_low();
    scl_low();
}

/* i2c_stop: 发送软件 I2C 停止条件。 */
static void i2c_stop(void)
{
    sda_low();
    scl_high();
    sda_high();
}

/* i2c_write: 写一个字节并忽略 ACK，课堂演示中以显示效果为准。 */
static void i2c_write(uint8_t value)
{
    uint8_t mask;

    for (mask = 0x80u; mask != 0u; mask >>= 1) {
        if ((value & mask) != 0u) {
            sda_high();
        } else {
            sda_low();
        }
        scl_high();
        scl_low();
    }

    sda_high();
    scl_high();
    scl_low();
}

/* oled_cmd: 写 SSD1306 命令字节。 */
static void oled_cmd(uint8_t command)
{
    i2c_start();
    i2c_write(OLED_ADDR);
    i2c_write(0x00u);
    i2c_write(command);
    i2c_stop();
}

/* oled_data: 写 SSD1306 数据字节。 */
static void oled_data(uint8_t data)
{
    i2c_start();
    i2c_write(OLED_ADDR);
    i2c_write(0x40u);
    i2c_write(data);
    i2c_stop();
}

/* oled_set_pos: 设置页地址和列地址。 */
static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_cmd((uint8_t)(0xB0u | page));
    oled_cmd((uint8_t)(0x00u | (col & 0x0Fu)));
    oled_cmd((uint8_t)(0x10u | ((col >> 4) & 0x0Fu)));
}

/* font_columns: 返回字符的 5 列点阵。 */
static void font_columns(char ch, uint8_t out[5])
{
    uint8_t i;
    static const uint8_t blank[5] = {0, 0, 0, 0, 0};
    const uint8_t *p;

    static const uint8_t n0[5] = {0x3E,0x51,0x49,0x45,0x3E};
    static const uint8_t n1[5] = {0x00,0x42,0x7F,0x40,0x00};
    static const uint8_t n2[5] = {0x42,0x61,0x51,0x49,0x46};
    static const uint8_t n3[5] = {0x21,0x41,0x45,0x4B,0x31};
    static const uint8_t n4[5] = {0x18,0x14,0x12,0x7F,0x10};
    static const uint8_t n5[5] = {0x27,0x45,0x45,0x45,0x39};
    static const uint8_t n6[5] = {0x3C,0x4A,0x49,0x49,0x30};
    static const uint8_t n7[5] = {0x01,0x71,0x09,0x05,0x03};
    static const uint8_t n8[5] = {0x36,0x49,0x49,0x49,0x36};
    static const uint8_t n9[5] = {0x06,0x49,0x49,0x29,0x1E};
    static const uint8_t font_a[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t font_c[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t font_d[5] = {0x7F,0x41,0x41,0x22,0x1C};
    static const uint8_t font_e[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t font_g[5] = {0x3E,0x41,0x49,0x49,0x7A};
    static const uint8_t font_h[5] = {0x7F,0x08,0x08,0x08,0x7F};
    static const uint8_t font_i[5] = {0x00,0x41,0x7F,0x41,0x00};
    static const uint8_t font_l[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t font_m[5] = {0x7F,0x02,0x0C,0x02,0x7F};
    static const uint8_t font_o[5] = {0x3E,0x41,0x41,0x41,0x3E};
    static const uint8_t font_p[5] = {0x7F,0x09,0x09,0x09,0x06};
    static const uint8_t font_r[5] = {0x7F,0x09,0x19,0x29,0x46};
    static const uint8_t font_s[5] = {0x46,0x49,0x49,0x49,0x31};
    static const uint8_t font_t[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t font_w[5] = {0x7F,0x20,0x18,0x20,0x7F};
    static const uint8_t percent[5] = {0x23,0x13,0x08,0x64,0x62};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t slash[5] = {0x20,0x10,0x08,0x04,0x02};

    p = blank;
    if ((ch >= 'a') && (ch <= 'z')) {
        ch = (char)(ch - 'a' + 'A');
    }
    switch (ch) {
    case '0': p = n0; break;
    case '1': p = n1; break;
    case '2': p = n2; break;
    case '3': p = n3; break;
    case '4': p = n4; break;
    case '5': p = n5; break;
    case '6': p = n6; break;
    case '7': p = n7; break;
    case '8': p = n8; break;
    case '9': p = n9; break;
    case 'A': p = font_a; break;
    case 'C': p = font_c; break;
    case 'D': p = font_d; break;
    case 'E': p = font_e; break;
    case 'G': p = font_g; break;
    case 'H': p = font_h; break;
    case 'I': p = font_i; break;
    case 'L': p = font_l; break;
    case 'M': p = font_m; break;
    case 'O': p = font_o; break;
    case 'P': p = font_p; break;
    case 'R': p = font_r; break;
    case 'S': p = font_s; break;
    case 'T': p = font_t; break;
    case 'W': p = font_w; break;
    case '%': p = percent; break;
    case ':': p = colon; break;
    case '.': p = dot; break;
    case '-': p = minus; break;
    case '/': p = slash; break;
    default: p = blank; break;
    }

    for (i = 0u; i < 5u; i++) {
        out[i] = p[i];
    }
}

/* oled_write_char: 在当前页写入一个 6 列宽字符。 */
static void oled_write_char(char ch)
{
    uint8_t cols[5];
    uint8_t i;

    font_columns(ch, cols);
    for (i = 0u; i < 5u; i++) {
        oled_data(cols[i]);
    }
    oled_data(0x00u);
}

/* oled_write_line: 写一行最多 16 个字符，不足部分用空格清除。 */
static void oled_write_line(uint8_t page, const char *text)
{
    uint8_t count;

    oled_set_pos(page, 0u);
    for (count = 0u; count < 21u; count++) {
        if ((text != 0) && (*text != '\0')) {
            oled_write_char(*text);
            text++;
        } else {
            oled_write_char(' ');
        }
    }
}

void oled_ssd1306_init(void)
{
    OLED_SEL &= (uint8_t)~(OLED_SDA_BIT | OLED_SCL_BIT);
    OLED_DIR |= OLED_SDA_BIT | OLED_SCL_BIT;
    OLED_REN &= (uint8_t)~(OLED_SDA_BIT | OLED_SCL_BIT);
    sda_high();
    scl_high();
    board_delay_ms(50u);

    oled_cmd(0xAEu);
    oled_cmd(0x20u);
    oled_cmd(0x02u);
    oled_cmd(0xB0u);
    oled_cmd(0xC8u);
    oled_cmd(0x00u);
    oled_cmd(0x10u);
    oled_cmd(0x40u);
    oled_cmd(0x81u);
    oled_cmd(0x7Fu);
    oled_cmd(0xA1u);
    oled_cmd(0xA6u);
    oled_cmd(0xA8u);
    oled_cmd(0x3Fu);
    oled_cmd(0xA4u);
    oled_cmd(0xD3u);
    oled_cmd(0x00u);
    oled_cmd(0xD5u);
    oled_cmd(0x80u);
    oled_cmd(0xD9u);
    oled_cmd(0xF1u);
    oled_cmd(0xDAu);
    oled_cmd(0x12u);
    oled_cmd(0xDBu);
    oled_cmd(0x40u);
    oled_cmd(0x8Du);
    oled_cmd(0x14u);
    oled_cmd(0xAFu);
    oled_ssd1306_clear();
}

void oled_ssd1306_clear(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0u; page < 8u; page++) {
        oled_set_pos(page, 0u);
        for (col = 0u; col < 128u; col++) {
            oled_data(0x00u);
        }
    }
}

void oled_ssd1306_show_lines(const char *line0, const char *line1, const char *line2, const char *line3)
{
    oled_write_line(0u, line0);
    oled_write_line(2u, line1);
    oled_write_line(4u, line2);
    oled_write_line(6u, line3);
}
