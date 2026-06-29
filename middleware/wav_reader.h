#ifndef WAV_READER_H
#define WAV_READER_H

#include <stdint.h>

#include "ff.h"

typedef struct {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t block_align;
    uint32_t data_start;
    uint32_t data_bytes;
} WavInfo;

typedef enum {
    WAV_RESULT_OK = 0,
    WAV_RESULT_FILE_ERROR,
    WAV_RESULT_NOT_WAVE,
    WAV_RESULT_UNSUPPORTED
} WavResult;

WavResult wav_reader_open(FIL *file, WavInfo *info);
const char *wav_reader_result_text(WavResult result);

#endif

