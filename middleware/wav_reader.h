/*
 * wav_reader.h
 * Minimal RIFF/WAV parser for TF-card audio files. The player only accepts
 * uncompressed 16-bit PCM because that format can be streamed on the MSP430
 * without a decoder heap or large RAM buffers.
 */
#ifndef WAV_READER_H
#define WAV_READER_H

#include <stdint.h>

#include "ff.h"

/* WavInfo: parsed format and data-location metadata for one WAV file. */
typedef struct {
    /* audio_format: WAV format code; 1 means uncompressed PCM. */
    uint16_t audio_format;
    /* channels: 1 for mono, 2 for stereo. */
    uint16_t channels;
    /* sample_rate: samples per second from the fmt chunk. */
    uint32_t sample_rate;
    /* bits_per_sample: PCM width; this project accepts 16 only. */
    uint16_t bits_per_sample;
    /* block_align: bytes per audio frame across all channels. */
    uint16_t block_align;
    /* data_start: file offset where PCM payload begins. */
    uint32_t data_start;
    /* data_bytes: number of PCM payload bytes available. */
    uint32_t data_bytes;
} WavInfo;

/* WavResult: parser result used for user-facing Bluetooth diagnostics. */
typedef enum {
    /* WAV_RESULT_OK: WAV file is supported and info was filled. */
    WAV_RESULT_OK = 0,
    /* WAV_RESULT_FILE_ERROR: FatFs read/seek failed while parsing. */
    WAV_RESULT_FILE_ERROR,
    /* WAV_RESULT_NOT_WAVE: RIFF/WAVE structure or required chunks were missing. */
    WAV_RESULT_NOT_WAVE,
    /* WAV_RESULT_UNSUPPORTED: WAV is valid but not 16-bit PCM mono/stereo. */
    WAV_RESULT_UNSUPPORTED
} WavResult;

/* wav_reader_open: parses file from its current start and fills *info on success. */
WavResult wav_reader_open(FIL *file, WavInfo *info);

/* wav_reader_result_text: returns a short constant diagnostic string for result. */
const char *wav_reader_result_text(WavResult result);

#endif
