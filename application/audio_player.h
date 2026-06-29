/*
 * audio_player.h
 * Top-level player state machine for the course project. It combines TF-card
 * WAV reading, software I2S output, EC11/local button control, and Bluetooth
 * serial commands into one foreground service.
 */
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>

/* PlayerMode: public playback state for diagnostics and future display code. */
typedef enum {
    /* PLAYER_MODE_STOPPED: file is open or selected but playback is reset/stopped. */
    PLAYER_MODE_STOPPED = 0,
    /* PLAYER_MODE_PLAYING: service loop is actively reading WAV bytes and outputting I2S. */
    PLAYER_MODE_PLAYING,
    /* PLAYER_MODE_PAUSED: file position is preserved and audio output is silent. */
    PLAYER_MODE_PAUSED,
    /* PLAYER_MODE_ERROR: player stopped because SD, WAV, or seek/read failed. */
    PLAYER_MODE_ERROR
} PlayerMode;

/* audio_player_init: mounts TF card, opens the first playable track, and reports boot status. */
void audio_player_init(void);

/* audio_player_poll_controls: consumes Bluetooth, EC11, and local button events. */
void audio_player_poll_controls(void);

/* audio_player_service: streams a short audio chunk when the player is in PLAYING mode. */
void audio_player_service(void);

/* audio_player_mode: returns the current public playback state. */
PlayerMode audio_player_mode(void);

#endif
