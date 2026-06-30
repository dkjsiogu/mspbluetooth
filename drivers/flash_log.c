/*
 * flash_log.c
 * 使用 MSP430 信息 Flash 保存环境历史记录。当前实现采用追加写入，
 * 信息段写满后整体擦除并从新序号继续记录，适合课程演示和独立读出。
 */
#include "flash_log.h"

#include <msp430.h>

#include "board.h"

/* FLASH_LOG_BASE: 信息段 D/C/B 起始地址，避开通常保存校准数据的信息段 A。 */
#define FLASH_LOG_BASE                  ((uint8_t *)0x1800u)

/* FLASH_LOG_BYTES: 使用 3 个 128 字节信息段保存历史记录。 */
#define FLASH_LOG_BYTES                 384u

/* FLASH_LOG_CAPACITY: 当前结构在信息段中能保存的记录条数。 */
#define FLASH_LOG_CAPACITY              (FLASH_LOG_BYTES / sizeof(FlashLogRecord))

/* g_log_count: 上电扫描得到的可读记录数。 */
static uint16_t g_log_count = 0u;

/* g_next_seq: 下一条写入记录的序号。 */
static uint32_t g_next_seq = 1u;

/* record_at: 返回指定槽位对应的 Flash 记录指针。 */
static const FlashLogRecord *record_at(uint16_t slot)
{
    return (const FlashLogRecord *)(const void *)(FLASH_LOG_BASE + ((uint32_t)slot * sizeof(FlashLogRecord)));
}

/* record_write_at: 返回指定槽位对应的可写 Flash 记录指针。 */
static FlashLogRecord *record_write_at(uint16_t slot)
{
    return (FlashLogRecord *)(void *)(FLASH_LOG_BASE + ((uint32_t)slot * sizeof(FlashLogRecord)));
}

/* record_crc: 对记录字段做简单 16 位求和校验。 */
static uint16_t record_crc(const FlashLogRecord *record)
{
    const uint8_t *bytes;
    uint16_t index;
    uint16_t total;

    bytes = (const uint8_t *)(const void *)record;
    total = 0u;
    for (index = 0u; index < (uint16_t)(sizeof(FlashLogRecord) - sizeof(uint16_t)); index++) {
        total = (uint16_t)(total + bytes[index]);
    }
    return total;
}

/* flash_write_word: 向 Flash 写入一个 16 位字。 */
static void flash_write_word(uint16_t *address, uint16_t value)
{
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | WRT;
    *address = value;
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
}

/* flash_write_record: 逐字写入一条记录。 */
static void flash_write_record(uint16_t slot, const FlashLogRecord *record)
{
    uint16_t word;
    const uint16_t *src;
    uint16_t *dst;

    src = (const uint16_t *)(const void *)record;
    dst = (uint16_t *)(void *)record_write_at(slot);
    for (word = 0u; word < (uint16_t)(sizeof(FlashLogRecord) / 2u); word++) {
        flash_write_word(&dst[word], src[word]);
    }
}

/* flash_erase_segment: 擦除 128 字节信息段。 */
static void flash_erase_segment(uint8_t *address)
{
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *address = 0u;
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
}

void flash_log_clear(void)
{
    uint16_t gie;

    gie = (uint16_t)(__get_SR_register() & GIE);
    __disable_interrupt();
    flash_erase_segment(FLASH_LOG_BASE);
    flash_erase_segment(FLASH_LOG_BASE + 128u);
    flash_erase_segment(FLASH_LOG_BASE + 256u);
    if (gie != 0u) {
        __enable_interrupt();
    }

    g_log_count = 0u;
    g_next_seq = 1u;
}

void flash_log_init(void)
{
    uint16_t slot;
    const FlashLogRecord *record;

    g_log_count = 0u;
    g_next_seq = 1u;

    for (slot = 0u; slot < FLASH_LOG_CAPACITY; slot++) {
        record = record_at(slot);
        if ((record->magic == FLASH_LOG_MAGIC) && (record_crc(record) == record->crc)) {
            g_log_count++;
            if (record->seq >= g_next_seq) {
                g_next_seq = record->seq + 1u;
            }
        } else {
            break;
        }
    }
}

void flash_log_append(const EnvSensorSample *sample, int16_t threshold_x10, uint8_t alarm_level)
{
    FlashLogRecord record;

    if (g_log_count >= FLASH_LOG_CAPACITY) {
        flash_log_clear();
    }

    record.magic = FLASH_LOG_MAGIC;
    record.seq = g_next_seq;
    record.uptime_s = board_millis() / 1000UL;
    record.temperature_x10 = sample->temperature_x10;
    record.humidity_x10 = sample->humidity_x10;
    record.gas_adc = sample->gas_adc;
    record.distance_mm = sample->distance_mm;
    record.threshold_x10 = threshold_x10;
    record.alarm_level = alarm_level;
    record.flags = sample->flags;
    record.crc = record_crc(&record);

    flash_write_record(g_log_count, &record);
    g_log_count++;
    g_next_seq++;
}

uint16_t flash_log_count(void)
{
    return g_log_count;
}

uint8_t flash_log_read(uint16_t index, FlashLogRecord *record)
{
    const FlashLogRecord *source;

    if ((index == 0u) || (index > g_log_count)) {
        return 0u;
    }

    source = record_at((uint16_t)(index - 1u));
    if ((source->magic != FLASH_LOG_MAGIC) || (record_crc(source) != source->crc)) {
        return 0u;
    }

    *record = *source;
    return 1u;
}
