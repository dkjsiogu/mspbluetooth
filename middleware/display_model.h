/*
 * display_model.h
 * Hardware-independent status display model. It formats the player state into
 * three short ASCII lines that can be mirrored over Bluetooth today and later
 * drawn on an e-paper panel if the hardware pin map is redesigned.
 */
#ifndef DISPLAY_MODEL_H
#define DISPLAY_MODEL_H

#include <stdint.h>

/* DISPLAY_MODEL_LINE_CHARS: visible characters stored per display line. */
#define DISPLAY_MODEL_LINE_CHARS        24u

/* DISPLAY_MODEL_LINE_BYTES: line buffer bytes including the null terminator. */
#define DISPLAY_MODEL_LINE_BYTES        (DISPLAY_MODEL_LINE_CHARS + 1u)

/* DisplayModelInput: compact data required to render the player status frame. */
typedef struct {
    /* mode_text: stable ASCII playback mode such as playing, paused, or error. */
    const char *mode_text;
    /* order_text: compact playback order text such as SEQ, ALL, or ONE. */
    const char *order_text;
    /* track_index: current numbered TRACKxx.WAV selection. */
    uint8_t track_index;
    /* volume: current software volume step. */
    uint8_t volume;
    /* sd_ready: nonzero when FatFs mount succeeded. */
    uint8_t sd_ready;
    /* file_open: nonzero when a supported WAV file is open. */
    uint8_t file_open;
    /* sample_rate: current WAV sample rate, or 0 when no file is open. */
    uint32_t sample_rate;
    /* channels: current WAV channel count, or 0 when no file is open. */
    uint8_t channels;
    /* progress_percent: current track playback progress, clamped to 0..100. */
    uint8_t progress_percent;
} DisplayModelInput;

/* DisplayFrame: three lines suitable for a small display or Bluetooth mirror. */
typedef struct {
    /* line1: transport state, track number, and volume. */
    char line1[DISPLAY_MODEL_LINE_BYTES];
    /* line2: SD/file readiness summary. */
    char line2[DISPLAY_MODEL_LINE_BYTES];
    /* line3: sample-rate/channel/progress or operator hint. */
    char line3[DISPLAY_MODEL_LINE_BYTES];
} DisplayFrame;

/*
 * display_model_build: formats player state into fixed-size display lines.
 * input: source player state snapshot to render.
 * frame: output three-line display frame.
 */
void display_model_build(const DisplayModelInput *input, DisplayFrame *frame);

#endif
