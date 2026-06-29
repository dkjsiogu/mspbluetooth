#include "audio_player.h"

#include "bluetooth_uart.h"
#include "board.h"
#include "encoder.h"
#include "ff.h"
#include "i2s_dac.h"
#include "platform_config.h"
#include "wav_reader.h"

typedef struct {
    PlayerMode mode;
    uint8_t sd_ready;
    uint8_t file_open;
    uint8_t track_index;
    uint8_t volume;
    uint16_t bytes_per_frame;
    uint32_t data_remaining;
    uint32_t led_stamp_ms;
    WavInfo wav;
} PlayerState;

static FATFS g_fatfs;
static FIL g_audio_file;
static PlayerState g_player;
static uint8_t g_audio_buffer[PLAYER_AUDIO_BUFFER_BYTES];

static void player_write_prompt(void);
static uint8_t player_open_track(uint8_t track_index, uint8_t start_playing);

static int16_t read_sample16(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static void make_track_name(uint8_t track_index, char *name)
{
    name[0] = 'T';
    name[1] = 'R';
    name[2] = 'A';
    name[3] = 'C';
    name[4] = 'K';
    name[5] = (char)('0' + (track_index / 10u));
    name[6] = (char)('0' + (track_index % 10u));
    name[7] = '.';
    name[8] = 'W';
    name[9] = 'A';
    name[10] = 'V';
    name[11] = '\0';
}

static const char *mode_text(PlayerMode mode)
{
    switch (mode) {
    case PLAYER_MODE_STOPPED:
        return "stopped";
    case PLAYER_MODE_PLAYING:
        return "playing";
    case PLAYER_MODE_PAUSED:
        return "paused";
    case PLAYER_MODE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static void player_report_status(void)
{
    bluetooth_uart_write_str("status=");
    bluetooth_uart_write_str(mode_text(g_player.mode));
    bluetooth_uart_write_str(" track=");
    bluetooth_uart_write_uint(g_player.track_index);
    bluetooth_uart_write_str(" volume=");
    bluetooth_uart_write_uint(g_player.volume);
    if (g_player.file_open != 0u) {
        bluetooth_uart_write_str(" rate=");
        bluetooth_uart_write_uint(g_player.wav.sample_rate);
        bluetooth_uart_write_str("Hz channels=");
        bluetooth_uart_write_uint(g_player.wav.channels);
    }
    bluetooth_uart_write_str("\r\n");
}

static void player_report_track_opened(void)
{
    char name[12];

    make_track_name(g_player.track_index, name);
    bluetooth_uart_write_str("open ");
    bluetooth_uart_write_str(name);
    bluetooth_uart_write_str(" ");
    bluetooth_uart_write_uint(g_player.wav.sample_rate);
    bluetooth_uart_write_str("Hz ");
    bluetooth_uart_write_uint(g_player.wav.channels);
    bluetooth_uart_write_str("ch\r\n");
}

static void player_close_file(void)
{
    if (g_player.file_open != 0u) {
        (void)f_close(&g_audio_file);
        g_player.file_open = 0u;
    }
}

static void player_set_error(const char *message)
{
    player_close_file();
    g_player.mode = PLAYER_MODE_ERROR;
    bluetooth_uart_write_str("error: ");
    bluetooth_uart_write_line(message);
}

static uint8_t player_open_track(uint8_t track_index, uint8_t start_playing)
{
    char name[12];
    FRESULT file_result;
    WavResult wav_result;

    if ((track_index == 0u) || (track_index > PLAYER_MAX_TRACKS)) {
        return 0u;
    }

    player_close_file();
    make_track_name(track_index, name);

    file_result = f_open(&g_audio_file, name, FA_READ | FA_OPEN_EXISTING);
    if (file_result != FR_OK) {
        return 0u;
    }

    wav_result = wav_reader_open(&g_audio_file, &g_player.wav);
    if (wav_result != WAV_RESULT_OK) {
        (void)f_close(&g_audio_file);
        g_player.file_open = 0u;
        bluetooth_uart_write_str("reject ");
        bluetooth_uart_write_str(name);
        bluetooth_uart_write_str(": ");
        bluetooth_uart_write_line(wav_reader_result_text(wav_result));
        return 0u;
    }

    g_player.file_open = 1u;
    g_player.track_index = track_index;
    g_player.bytes_per_frame = (uint16_t)(g_player.wav.channels * 2u);
    g_player.data_remaining = g_player.wav.data_bytes;
    g_player.mode = (start_playing != 0u) ? PLAYER_MODE_PLAYING : PLAYER_MODE_PAUSED;
    player_report_track_opened();
    return 1u;
}

static uint8_t player_open_near_track(uint8_t start_index, int8_t direction)
{
    uint8_t attempts;
    uint8_t index;

    index = start_index;
    attempts = 0u;

    while (attempts < PLAYER_MAX_TRACKS) {
        if (index == 0u) {
            index = PLAYER_MAX_TRACKS;
        } else if (index > PLAYER_MAX_TRACKS) {
            index = 1u;
        }

        if (player_open_track(index, 1u) != 0u) {
            return 1u;
        }

        if (direction >= 0) {
            index++;
        } else {
            index--;
        }
        attempts++;
    }

    return 0u;
}

static void player_next_track(void)
{
    if (player_open_near_track((uint8_t)(g_player.track_index + 1u), 1) == 0u) {
        player_set_error("no playable track");
    }
}

static void player_previous_track(void)
{
    if (player_open_near_track((uint8_t)(g_player.track_index - 1u), -1) == 0u) {
        player_set_error("no playable track");
    }
}

static void player_play(void)
{
    if (g_player.file_open == 0u) {
        if (player_open_near_track(g_player.track_index, 1) == 0u) {
            player_set_error("no playable track");
        }
        return;
    }

    if (g_player.mode == PLAYER_MODE_STOPPED) {
        if (f_lseek(&g_audio_file, g_player.wav.data_start) != FR_OK) {
            player_set_error("seek failed");
            return;
        }
        g_player.data_remaining = g_player.wav.data_bytes;
    }

    g_player.mode = PLAYER_MODE_PLAYING;
    bluetooth_uart_write_line("play");
}

static void player_pause(void)
{
    if (g_player.mode == PLAYER_MODE_PLAYING) {
        g_player.mode = PLAYER_MODE_PAUSED;
        i2s_dac_write_silence(8u);
        bluetooth_uart_write_line("pause");
    }
}

static void player_toggle_play_pause(void)
{
    if (g_player.mode == PLAYER_MODE_PLAYING) {
        player_pause();
    } else {
        player_play();
    }
}

static void player_stop(void)
{
    if (g_player.file_open != 0u) {
        (void)f_lseek(&g_audio_file, g_player.wav.data_start);
        g_player.data_remaining = g_player.wav.data_bytes;
    }
    g_player.mode = PLAYER_MODE_STOPPED;
    i2s_dac_write_silence(8u);
    bluetooth_uart_write_line("stop");
}

static void player_set_volume(uint8_t volume)
{
    if (volume > PLAYER_VOLUME_MAX) {
        volume = PLAYER_VOLUME_MAX;
    }

    g_player.volume = volume;
    i2s_dac_set_volume(volume);
    bluetooth_uart_write_str("volume=");
    bluetooth_uart_write_uint(volume);
    bluetooth_uart_write_str("\r\n");
}

static void player_volume_up(void)
{
    if (g_player.volume < PLAYER_VOLUME_MAX) {
        player_set_volume((uint8_t)(g_player.volume + 1u));
    }
}

static void player_volume_down(void)
{
    if (g_player.volume > 0u) {
        player_set_volume((uint8_t)(g_player.volume - 1u));
    }
}

static void player_handle_command(uint8_t command)
{
    if ((command >= (uint8_t)'A') && (command <= (uint8_t)'Z')) {
        command = (uint8_t)(command + ((uint8_t)'a' - (uint8_t)'A'));
    }

    if ((command >= (uint8_t)'1') && (command <= (uint8_t)'9')) {
        if (player_open_track((uint8_t)(command - (uint8_t)'0'), 1u) == 0u) {
            bluetooth_uart_write_line("track not found");
        }
        return;
    }

    switch (command) {
    case 'p':
        player_toggle_play_pause();
        break;
    case 's':
        player_stop();
        break;
    case 'n':
    case '>':
        player_next_track();
        break;
    case 'b':
    case '<':
        player_previous_track();
        break;
    case '+':
    case '=':
        player_volume_up();
        break;
    case '-':
    case '_':
        player_volume_down();
        break;
    case '?':
        player_report_status();
        break;
    case 'h':
        player_write_prompt();
        break;
    default:
        break;
    }
}

static void player_write_prompt(void)
{
    bluetooth_uart_write_line("cmd: p play/pause, s stop, n next, b prev, +/- volume, 1-9 track, ? status");
}

void audio_player_init(void)
{
    FRESULT mount_result;

    g_player.mode = PLAYER_MODE_STOPPED;
    g_player.sd_ready = 0u;
    g_player.file_open = 0u;
    g_player.track_index = 1u;
    g_player.volume = PLAYER_DEFAULT_VOLUME;
    g_player.bytes_per_frame = 4u;
    g_player.data_remaining = 0u;
    g_player.led_stamp_ms = board_millis();

    i2s_dac_set_volume(g_player.volume);

    mount_result = f_mount(0u, &g_fatfs);
    if (mount_result != FR_OK) {
        player_set_error("sd mount failed");
        player_write_prompt();
        return;
    }

    g_player.sd_ready = 1u;
    bluetooth_uart_write_line("sd mounted");
    player_write_prompt();

    if (player_open_near_track(1u, 1) == 0u) {
        player_set_error("put TRACK01.WAV on TF card");
        return;
    }

#if PLAYER_AUTOPLAY_ON_BOOT == 0
    g_player.mode = PLAYER_MODE_PAUSED;
#endif
}

void audio_player_poll_controls(void)
{
    uint8_t command;
    uint8_t events;

    while (bluetooth_uart_read(&command) != 0u) {
        player_handle_command(command);
    }

    events = encoder_take_events();
    if ((events & ENCODER_EVENT_CW) != 0u) {
        player_volume_up();
    }
    if ((events & ENCODER_EVENT_CCW) != 0u) {
        player_volume_down();
    }
    if ((events & ENCODER_EVENT_BUTTON) != 0u) {
        player_toggle_play_pause();
    }

    if (board_elapsed_ms(&g_player.led_stamp_ms, 500u) != 0u) {
        board_status_led_toggle();
    }
}

void audio_player_service(void)
{
    UINT bytes_read;
    UINT request_bytes;
    UINT offset;
    int16_t left;
    int16_t right;
    FRESULT read_result;

    if (g_player.mode != PLAYER_MODE_PLAYING) {
        return;
    }

    if ((g_player.file_open == 0u) || (g_player.data_remaining == 0u)) {
        player_next_track();
        return;
    }

    request_bytes = PLAYER_AUDIO_BUFFER_BYTES;
    if (g_player.data_remaining < (uint32_t)request_bytes) {
        request_bytes = (UINT)g_player.data_remaining;
    }
    request_bytes = (UINT)(request_bytes - (request_bytes % g_player.bytes_per_frame));
    if (request_bytes == 0u) {
        player_next_track();
        return;
    }

    bytes_read = 0u;
    read_result = f_read(&g_audio_file, g_audio_buffer, request_bytes, &bytes_read);
    if ((read_result != FR_OK) || (bytes_read == 0u)) {
        player_set_error("sd read failed");
        return;
    }

    if (bytes_read > g_player.data_remaining) {
        g_player.data_remaining = 0u;
    } else {
        g_player.data_remaining -= bytes_read;
    }

    offset = 0u;
    while ((offset + g_player.bytes_per_frame) <= bytes_read) {
        left = read_sample16(&g_audio_buffer[offset]);
        if (g_player.wav.channels == 2u) {
            right = read_sample16(&g_audio_buffer[offset + 2u]);
        } else {
            right = left;
        }

        i2s_dac_write_stereo(left, right);
        offset = (UINT)(offset + g_player.bytes_per_frame);
    }

    if (g_player.data_remaining == 0u) {
        player_next_track();
    }
}

PlayerMode audio_player_mode(void)
{
    return g_player.mode;
}

