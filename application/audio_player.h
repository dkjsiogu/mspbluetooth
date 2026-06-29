#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>

typedef enum {
    PLAYER_MODE_STOPPED = 0,
    PLAYER_MODE_PLAYING,
    PLAYER_MODE_PAUSED,
    PLAYER_MODE_ERROR
} PlayerMode;

void audio_player_init(void);
void audio_player_poll_controls(void);
void audio_player_service(void);
PlayerMode audio_player_mode(void);

#endif

