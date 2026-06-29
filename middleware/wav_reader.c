#include "wav_reader.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint8_t read_exact(FIL *file, void *buffer, UINT length)
{
    UINT bytes_read;

    bytes_read = 0u;
    if (f_read(file, buffer, length, &bytes_read) != FR_OK) {
        return 0u;
    }

    return (uint8_t)(bytes_read == length);
}

static uint8_t skip_bytes(FIL *file, uint32_t count)
{
    return (uint8_t)(f_lseek(file, f_tell(file) + count) == FR_OK);
}

static WavResult validate_info(const WavInfo *info)
{
    uint16_t expected_align;

    if (info->audio_format != 1u) {
        return WAV_RESULT_UNSUPPORTED;
    }
    if ((info->channels != 1u) && (info->channels != 2u)) {
        return WAV_RESULT_UNSUPPORTED;
    }
    if (info->bits_per_sample != 16u) {
        return WAV_RESULT_UNSUPPORTED;
    }

    expected_align = (uint16_t)(info->channels * 2u);
    if (info->block_align != expected_align) {
        return WAV_RESULT_UNSUPPORTED;
    }

    return WAV_RESULT_OK;
}

WavResult wav_reader_open(FIL *file, WavInfo *info)
{
    uint8_t header[12];
    uint8_t chunk_header[8];
    uint8_t fmt_data[32];
    uint32_t chunk_size;
    uint32_t bytes_to_read;
    uint8_t found_fmt;

    memset(info, 0, sizeof(*info));

    if (read_exact(file, header, sizeof(header)) == 0u) {
        return WAV_RESULT_FILE_ERROR;
    }

    if ((memcmp(header, "RIFF", 4u) != 0) || (memcmp(&header[8], "WAVE", 4u) != 0)) {
        return WAV_RESULT_NOT_WAVE;
    }

    found_fmt = 0u;
    while (f_tell(file) < f_size(file)) {
        if (read_exact(file, chunk_header, sizeof(chunk_header)) == 0u) {
            return WAV_RESULT_FILE_ERROR;
        }

        chunk_size = read_le32(&chunk_header[4]);
        if (memcmp(chunk_header, "fmt ", 4u) == 0) {
            if (chunk_size < 16u) {
                return WAV_RESULT_NOT_WAVE;
            }

            bytes_to_read = chunk_size;
            if (bytes_to_read > sizeof(fmt_data)) {
                bytes_to_read = sizeof(fmt_data);
            }

            if (read_exact(file, fmt_data, (UINT)bytes_to_read) == 0u) {
                return WAV_RESULT_FILE_ERROR;
            }
            if (chunk_size > bytes_to_read) {
                if (skip_bytes(file, chunk_size - bytes_to_read) == 0u) {
                    return WAV_RESULT_FILE_ERROR;
                }
            }
            if ((chunk_size & 1u) != 0u) {
                if (skip_bytes(file, 1u) == 0u) {
                    return WAV_RESULT_FILE_ERROR;
                }
            }

            info->audio_format = read_le16(&fmt_data[0]);
            info->channels = read_le16(&fmt_data[2]);
            info->sample_rate = read_le32(&fmt_data[4]);
            info->block_align = read_le16(&fmt_data[12]);
            info->bits_per_sample = read_le16(&fmt_data[14]);
            found_fmt = 1u;
        } else if (memcmp(chunk_header, "data", 4u) == 0) {
            if (found_fmt == 0u) {
                return WAV_RESULT_NOT_WAVE;
            }

            info->data_start = f_tell(file);
            info->data_bytes = chunk_size;
            if (f_lseek(file, info->data_start) != FR_OK) {
                return WAV_RESULT_FILE_ERROR;
            }

            return validate_info(info);
        } else {
            if (skip_bytes(file, chunk_size + (chunk_size & 1u)) == 0u) {
                return WAV_RESULT_FILE_ERROR;
            }
        }
    }

    return WAV_RESULT_NOT_WAVE;
}

const char *wav_reader_result_text(WavResult result)
{
    switch (result) {
    case WAV_RESULT_OK:
        return "ok";
    case WAV_RESULT_FILE_ERROR:
        return "file error";
    case WAV_RESULT_NOT_WAVE:
        return "not wav";
    case WAV_RESULT_UNSUPPORTED:
        return "unsupported wav";
    default:
        return "unknown wav error";
    }
}
