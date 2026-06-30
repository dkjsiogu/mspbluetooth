/*
 * env_monitor.c
 * 环境监测系统主逻辑。该文件集中处理周期采样、OLED 页面刷新、蓝牙
 * 文本协议、内部 Flash 日志和报警等级计算，便于课程验收时追踪流程。
 */
#include "env_monitor.h"

#include <stdint.h>

#include "alarm_buzzer.h"
#include "bluetooth_uart.h"
#include "board.h"
#include "encoder.h"
#include "env_sensors.h"
#include "flash_log.h"
#include "oled_ssd1306.h"
#include "platform_config.h"

/* CMD_BUFFER_SIZE: 蓝牙命令缓冲区长度。 */
#define CMD_BUFFER_SIZE                 24u

/* g_sample: 最近一次环境采样结果。 */
static EnvSensorSample g_sample;

/* g_temp_threshold_x10: 温度报警阈值，单位 0.1 摄氏度。 */
static int16_t g_temp_threshold_x10 = ENV_DEFAULT_TEMP_THRESHOLD_X10;

/* g_alarm_level: 当前综合报警等级。 */
static uint8_t g_alarm_level = 0u;

/* g_sample_stamp_ms: 周期采样时间戳。 */
static uint32_t g_sample_stamp_ms = 0u;

/* g_oled_stamp_ms: OLED 刷新时间戳。 */
static uint32_t g_oled_stamp_ms = 0u;

/* g_bt_stamp_ms: 蓝牙主动上传时间戳。 */
static uint32_t g_bt_stamp_ms = 0u;

/* g_flash_stamp_ms: Flash 历史记录写入时间戳。 */
static uint32_t g_flash_stamp_ms = 0u;

/* g_cmd_buffer: 蓝牙命令行缓冲区。 */
static char g_cmd_buffer[CMD_BUFFER_SIZE];

/* g_cmd_len: 当前蓝牙命令缓冲区长度。 */
static uint8_t g_cmd_len = 0u;

/* g_trace: 最近命令轨迹，便于手机端确认控制链路。 */
static char g_trace[ENV_TRACE_DEPTH][8];

/* g_trace_count: 已保存的轨迹条数。 */
static uint8_t g_trace_count = 0u;

/* str_eq: 比较两个零结尾字符串是否相等。 */
static uint8_t str_eq(const char *a, const char *b)
{
    while ((*a != '\0') && (*b != '\0')) {
        if (*a != *b) {
            return 0u;
        }
        a++;
        b++;
    }
    return (uint8_t)((*a == '\0') && (*b == '\0'));
}

/* str_starts: 判断 text 是否以 prefix 开头。 */
static uint8_t str_starts(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (*text != *prefix) {
            return 0u;
        }
        text++;
        prefix++;
    }
    return 1u;
}

/* append_char: 向有限缓冲区追加一个字符。 */
static void append_char(char *buffer, uint8_t *pos, uint8_t limit, char ch)
{
    if (*pos < (uint8_t)(limit - 1u)) {
        buffer[*pos] = ch;
        *pos = (uint8_t)(*pos + 1u);
        buffer[*pos] = '\0';
    }
}

/* append_str: 向有限缓冲区追加字符串。 */
static void append_str(char *buffer, uint8_t *pos, uint8_t limit, const char *text)
{
    while (*text != '\0') {
        append_char(buffer, pos, limit, *text);
        text++;
    }
}

/* append_uint: 向有限缓冲区追加无符号十进制数。 */
static void append_uint(char *buffer, uint8_t *pos, uint8_t limit, uint32_t value)
{
    char digits[10];
    uint8_t count;

    count = 0u;
    if (value == 0u) {
        append_char(buffer, pos, limit, '0');
        return;
    }

    while ((value > 0u) && (count < (uint8_t)sizeof(digits))) {
        digits[count] = (char)('0' + (char)(value % 10u));
        value /= 10u;
        count++;
    }
    while (count > 0u) {
        count--;
        append_char(buffer, pos, limit, digits[count]);
    }
}

/* append_x10: 向有限缓冲区追加 0.1 单位的有符号数。 */
static void append_x10(char *buffer, uint8_t *pos, uint8_t limit, int16_t value)
{
    uint16_t abs_value;

    if (value < 0) {
        append_char(buffer, pos, limit, '-');
        abs_value = (uint16_t)(-value);
    } else {
        abs_value = (uint16_t)value;
    }
    append_uint(buffer, pos, limit, abs_value / 10u);
    append_char(buffer, pos, limit, '.');
    append_uint(buffer, pos, limit, abs_value % 10u);
}

/* bt_write_x10: 通过蓝牙发送 0.1 单位的数值。 */
static void bt_write_x10(int16_t value)
{
    char line[12];
    uint8_t pos;

    pos = 0u;
    line[0] = '\0';
    append_x10(line, &pos, (uint8_t)sizeof(line), value);
    bluetooth_uart_write_str(line);
}

/* trace_add: 保存最近一次蓝牙或编码器动作。 */
static void trace_add(const char *text)
{
    uint8_t i;
    uint8_t j;

    if (g_trace_count < ENV_TRACE_DEPTH) {
        g_trace_count++;
    }
    for (i = (uint8_t)(g_trace_count - 1u); i > 0u; i--) {
        for (j = 0u; j < (uint8_t)sizeof(g_trace[0]); j++) {
            g_trace[i][j] = g_trace[i - 1u][j];
        }
    }
    for (j = 0u; j < (uint8_t)(sizeof(g_trace[0]) - 1u); j++) {
        g_trace[0][j] = text[j];
        if (text[j] == '\0') {
            break;
        }
    }
    g_trace[0][sizeof(g_trace[0]) - 1u] = '\0';
}

/* compute_alarm_level: 根据温度超限和气体等级计算综合报警等级。 */
static uint8_t compute_alarm_level(void)
{
    int16_t temp_over;
    uint8_t temp_level;
    uint8_t gas_level;

    temp_level = 0u;
    if ((g_sample.flags & ENV_SENSOR_DHT_OK) != 0u) {
        temp_over = (int16_t)(g_sample.temperature_x10 - g_temp_threshold_x10);
        if (temp_over > 0) {
            temp_level = (uint8_t)(1 + (temp_over / 20));
            if (temp_level > 5u) {
                temp_level = 5u;
            }
        }
    }

    gas_level = 0u;
    if ((g_sample.flags & ENV_SENSOR_MQ2_READY) != 0u) {
        gas_level = g_sample.gas_level;
    }

    return (temp_level > gas_level) ? temp_level : gas_level;
}

/* send_data_line: 通过蓝牙发送当前实时数据。 */
static void send_data_line(void)
{
    bluetooth_uart_write_str("DATA T=");
    bt_write_x10(g_sample.temperature_x10);
    bluetooth_uart_write_str(" H=");
    bluetooth_uart_write_uint(g_sample.humidity_x10 / 10u);
    bluetooth_uart_write_str(" GAS=");
    bluetooth_uart_write_uint(g_sample.gas_adc);
    bluetooth_uart_write_str(" L=");
    bluetooth_uart_write_uint(g_sample.gas_level);
    bluetooth_uart_write_str(" D=");
    bluetooth_uart_write_uint(g_sample.distance_mm);
    bluetooth_uart_write_str(" TH=");
    bt_write_x10(g_temp_threshold_x10);
    bluetooth_uart_write_str(" ALM=");
    bluetooth_uart_write_uint(g_alarm_level);
    bluetooth_uart_write_line("");
}

/* send_info: 发送固件信息。 */
static void send_info(void)
{
    bluetooth_uart_write_str("info name=");
    bluetooth_uart_write_str(PLAYER_FIRMWARE_NAME);
    bluetooth_uart_write_str(" version=");
    bluetooth_uart_write_str(PLAYER_FIRMWARE_VERSION);
    bluetooth_uart_write_str(" profile=");
    bluetooth_uart_write_line(PLAYER_HARDWARE_PROFILE);
}

/* send_wiring: 发送主要接线诊断信息。 */
static void send_wiring(void)
{
    bluetooth_uart_write_line("pin dht11 data=P1.0");
    bluetooth_uart_write_line("pin mq2 ao=P6.0 adc=A0");
    bluetooth_uart_write_line("pin hcsr04 trig=P1.2 echo=P1.3 note=echo-divide-to-3v3");
    bluetooth_uart_write_line("pin oled scl=P3.1 sda=P3.0 mode=soft-i2c");
    bluetooth_uart_write_line("pin bt tx=P4.4 rx=P4.5 mode=UCA1");
    bluetooth_uart_write_line("pin buzzer pwm=P2.0");
    bluetooth_uart_write_line("pin ec11 a=P2.1 b=P2.2 sw=P2.3");
}

/* send_help: 发送蓝牙命令说明。 */
static void send_help(void)
{
    bluetooth_uart_write_line("cmd ? i w x T+ T- SETT=32.0 HIST? HIST n DUMP CLRLOG");
}

/* send_trace: 发送最近控制轨迹。 */
static void send_trace(void)
{
    uint8_t i;

    bluetooth_uart_write_str("trace count=");
    bluetooth_uart_write_uint(g_trace_count);
    for (i = 0u; i < g_trace_count; i++) {
        bluetooth_uart_write_str(" ");
        bluetooth_uart_write_uint((uint32_t)(i + 1u));
        bluetooth_uart_write_str("=");
        bluetooth_uart_write_str(g_trace[i]);
    }
    bluetooth_uart_write_line("");
}

/* send_history_one: 发送指定历史记录。 */
static void send_history_one(uint16_t index)
{
    FlashLogRecord record;

    if (flash_log_read(index, &record) == 0u) {
        bluetooth_uart_write_line("ERR HIST");
        return;
    }

    bluetooth_uart_write_str("REC ");
    bluetooth_uart_write_uint(index);
    bluetooth_uart_write_str(" SEQ=");
    bluetooth_uart_write_uint(record.seq);
    bluetooth_uart_write_str(" T=");
    bt_write_x10(record.temperature_x10);
    bluetooth_uart_write_str(" H=");
    bluetooth_uart_write_uint(record.humidity_x10 / 10u);
    bluetooth_uart_write_str(" GAS=");
    bluetooth_uart_write_uint(record.gas_adc);
    bluetooth_uart_write_str(" D=");
    bluetooth_uart_write_uint(record.distance_mm);
    bluetooth_uart_write_str(" TH=");
    bt_write_x10(record.threshold_x10);
    bluetooth_uart_write_str(" ALM=");
    bluetooth_uart_write_uint(record.alarm_level);
    bluetooth_uart_write_line("");
}

/* send_history_summary: 发送历史记录数量。 */
static void send_history_summary(void)
{
    bluetooth_uart_write_str("HIST COUNT=");
    bluetooth_uart_write_uint(flash_log_count());
    bluetooth_uart_write_line("");
}

/* parse_uint16: 从字符串解析正整数。 */
static uint16_t parse_uint16(const char *text)
{
    uint16_t value;

    value = 0u;
    while ((*text >= '0') && (*text <= '9')) {
        value = (uint16_t)((value * 10u) + (uint16_t)(*text - '0'));
        text++;
    }
    return value;
}

/* parse_x10: 解析形如 32.0 的温度阈值。 */
static int16_t parse_x10(const char *text)
{
    int16_t value;
    uint8_t negative;

    value = 0;
    negative = 0u;
    if (*text == '-') {
        negative = 1u;
        text++;
    }
    while ((*text >= '0') && (*text <= '9')) {
        value = (int16_t)((value * 10) + (int16_t)(*text - '0'));
        text++;
    }
    value = (int16_t)(value * 10);
    if (*text == '.') {
        text++;
        if ((*text >= '0') && (*text <= '9')) {
            value = (int16_t)(value + (int16_t)(*text - '0'));
        }
    }
    return (negative != 0u) ? (int16_t)(-value) : value;
}

/* send_threshold: 发送当前温度报警阈值。 */
static void send_threshold(void)
{
    bluetooth_uart_write_str("TH=");
    bt_write_x10(g_temp_threshold_x10);
    bluetooth_uart_write_line("");
}

/* adjust_threshold: 调整温度报警阈值并刷新报警状态。 */
static void adjust_threshold(int16_t delta_x10, const char *trace)
{
    g_temp_threshold_x10 = (int16_t)(g_temp_threshold_x10 + delta_x10);
    if (g_temp_threshold_x10 < 0) {
        g_temp_threshold_x10 = 0;
    }
    if (g_temp_threshold_x10 > 800) {
        g_temp_threshold_x10 = 800;
    }
    trace_add(trace);
    send_threshold();
}

/* handle_command: 执行一条蓝牙文本命令。 */
static void handle_command(const char *command)
{
    uint16_t index;

    if (str_eq(command, "?")) {
        trace_add("bt:?");
        send_data_line();
    } else if (str_eq(command, "i") || str_eq(command, "I")) {
        trace_add("bt:info");
        send_info();
    } else if (str_eq(command, "w") || str_eq(command, "W")) {
        trace_add("bt:wire");
        send_wiring();
    } else if (str_eq(command, "h") || str_eq(command, "H")) {
        send_help();
    } else if (str_eq(command, "x") || str_eq(command, "X")) {
        trace_add("bt:trace");
        send_trace();
    } else if (str_eq(command, "T+")) {
        adjust_threshold(ENV_TEMP_STEP_X10, "bt:T+");
    } else if (str_eq(command, "T-")) {
        adjust_threshold((int16_t)(-ENV_TEMP_STEP_X10), "bt:T-");
    } else if (str_starts(command, "SETT=")) {
        g_temp_threshold_x10 = parse_x10(command + 5);
        trace_add("bt:SET");
        bluetooth_uart_write_str("TH=");
        bt_write_x10(g_temp_threshold_x10);
        bluetooth_uart_write_line(" SAVED");
    } else if (str_eq(command, "HIST?")) {
        trace_add("bt:H?");
        send_history_summary();
    } else if (str_starts(command, "HIST ")) {
        trace_add("bt:H");
        index = parse_uint16(command + 5);
        send_history_one(index);
    } else if (str_eq(command, "DUMP")) {
        trace_add("bt:DUMP");
        for (index = 1u; index <= flash_log_count(); index++) {
            send_history_one(index);
        }
        bluetooth_uart_write_line("DUMP END");
    } else if (str_eq(command, "CLRLOG")) {
        trace_add("bt:CLR");
        flash_log_clear();
        bluetooth_uart_write_line("LOG CLEARED");
    } else {
        bluetooth_uart_write_line("ERR CMD");
    }
}

/* poll_bluetooth: 接收蓝牙字节并按行或短命令触发解析。 */
static void poll_bluetooth(void)
{
    uint8_t byte;

    while (bluetooth_uart_read(&byte) != 0u) {
        if ((byte == '\r') || (byte == '\n')) {
            if (g_cmd_len > 0u) {
                g_cmd_buffer[g_cmd_len] = '\0';
                handle_command(g_cmd_buffer);
                g_cmd_len = 0u;
            }
        } else if ((byte == '?') || (byte == 'i') || (byte == 'I') ||
                   (byte == 'w') || (byte == 'W') || (byte == 'x') ||
                   (byte == 'X') || (byte == 'h') || (byte == 'H')) {
            g_cmd_buffer[0] = (char)byte;
            g_cmd_buffer[1] = '\0';
            handle_command(g_cmd_buffer);
            g_cmd_len = 0u;
        } else if (g_cmd_len < (uint8_t)(CMD_BUFFER_SIZE - 1u)) {
            g_cmd_buffer[g_cmd_len] = (char)byte;
            g_cmd_len++;
            g_cmd_buffer[g_cmd_len] = '\0';
            if ((g_cmd_len == 2u) && (g_cmd_buffer[0] == 'T') &&
                ((g_cmd_buffer[1] == '+') || (g_cmd_buffer[1] == '-'))) {
                handle_command(g_cmd_buffer);
                g_cmd_len = 0u;
            }
        } else {
            g_cmd_len = 0u;
            bluetooth_uart_write_line("ERR LONG");
        }
    }
}

/* make_line_real_time: 生成 OLED 第 1/2 行实时数据文本。 */
static void make_line_real_time(char *line0, char *line1)
{
    uint8_t pos;

    pos = 0u;
    line0[0] = '\0';
    append_str(line0, &pos, 22u, "T:");
    append_x10(line0, &pos, 22u, g_sample.temperature_x10);
    append_str(line0, &pos, 22u, "C H:");
    append_uint(line0, &pos, 22u, g_sample.humidity_x10 / 10u);
    append_char(line0, &pos, 22u, '%');

    pos = 0u;
    line1[0] = '\0';
    append_str(line1, &pos, 22u, "GAS:");
    append_uint(line1, &pos, 22u, g_sample.gas_adc);
    append_str(line1, &pos, 22u, " L:");
    append_uint(line1, &pos, 22u, g_sample.gas_level);
}

/* refresh_oled: 刷新 OLED 四行状态页面。 */
static void refresh_oled(void)
{
    char line0[22];
    char line1[22];
    char line2[22];
    char line3[22];
    uint8_t pos;

    make_line_real_time(line0, line1);

    pos = 0u;
    line2[0] = '\0';
    append_str(line2, &pos, 22u, "D:");
    if ((g_sample.flags & ENV_SENSOR_DISTANCE_OK) != 0u) {
        append_uint(line2, &pos, 22u, g_sample.distance_mm);
    } else {
        append_str(line2, &pos, 22u, "----");
    }
    append_str(line2, &pos, 22u, "MM");

    pos = 0u;
    line3[0] = '\0';
    append_str(line3, &pos, 22u, "TH:");
    append_x10(line3, &pos, 22u, g_temp_threshold_x10);
    append_str(line3, &pos, 22u, " ALM:");
    append_uint(line3, &pos, 22u, g_alarm_level);

    oled_ssd1306_show_lines(line0, line1, line2, line3);
}

/* poll_encoder: 处理 EC11 阈值调节和页面/保存动作。 */
static void poll_encoder(void)
{
    uint8_t events;

    events = encoder_take_events();
    if ((events & ENCODER_EVENT_CW) != 0u) {
        adjust_threshold(ENV_TEMP_STEP_X10, "enc:+");
    }
    if ((events & ENCODER_EVENT_CCW) != 0u) {
        adjust_threshold((int16_t)(-ENV_TEMP_STEP_X10), "enc:-");
    }
    if ((events & ENCODER_EVENT_BUTTON) != 0u) {
        trace_add("enc:sw");
        send_data_line();
    }
    if ((events & ENCODER_EVENT_BUTTON_LONG) != 0u) {
        trace_add("enc:save");
        bluetooth_uart_write_line("TH SAVED");
    }
}

void env_monitor_init(void)
{
    g_sample.temperature_x10 = 250;
    g_sample.humidity_x10 = 500u;
    g_sample.gas_adc = 0u;
    g_sample.gas_level = 0u;
    g_sample.distance_mm = 0u;
    g_sample.flags = 0u;

    env_sensors_init();
    oled_ssd1306_init();
    alarm_buzzer_init();
    flash_log_init();

    bluetooth_uart_write_line("");
    bluetooth_uart_write_line("MSP430F5529 ENV monitor");
    send_help();
}

void env_monitor_service(void)
{
    poll_bluetooth();
    poll_encoder();

    if (board_elapsed_ms(&g_sample_stamp_ms, ENV_SAMPLE_PERIOD_MS) != 0u) {
        env_sensors_sample(&g_sample);
        g_alarm_level = compute_alarm_level();
        alarm_buzzer_set_level(g_alarm_level);
        board_status_led_set((uint8_t)(g_alarm_level != 0u));
    }

    if (board_elapsed_ms(&g_oled_stamp_ms, ENV_OLED_REFRESH_MS) != 0u) {
        refresh_oled();
    }

    if (board_elapsed_ms(&g_bt_stamp_ms, ENV_BT_PUSH_MS) != 0u) {
        send_data_line();
    }

    if (board_elapsed_ms(&g_flash_stamp_ms, ENV_FLASH_LOG_MS) != 0u) {
        flash_log_append(&g_sample, g_temp_threshold_x10, g_alarm_level);
    }

    alarm_buzzer_service();
}
