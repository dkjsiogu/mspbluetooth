/*
 * oled_ssd1306.h
 * 128x64 SSD1306 OLED 显示驱动。模块提供初始化、清屏和四行文本刷新，
 * 用于显示温湿度、气体浓度、距离、阈值和报警状态。
 */
#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

/* oled_ssd1306_init: 初始化软件 I2C 引脚和 SSD1306 控制器。 */
void oled_ssd1306_init(void);

/* oled_ssd1306_clear: 清空 OLED 显存。 */
void oled_ssd1306_clear(void);

/*
 * oled_ssd1306_show_lines: 显示最多 4 行 16 字符文本。
 * line0: 第一行文本。
 * line1: 第二行文本。
 * line2: 第三行文本。
 * line3: 第四行文本。
 */
void oled_ssd1306_show_lines(const char *line0, const char *line1, const char *line2, const char *line3);

#endif
