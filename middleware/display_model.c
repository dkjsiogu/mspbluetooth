#include "display_model.h"

/* clear_line: fills line with a null-terminated empty string. */
static void clear_line(char *line)
{
    line[0] = '\0';
}

/* append_char: appends c to line when there is still visible capacity. */
static void append_char(char *line, char c)
{
    uint8_t index;

    index = 0u;
    while ((index < DISPLAY_MODEL_LINE_CHARS) && (line[index] != '\0')) {
        index++;
    }
    if (index < DISPLAY_MODEL_LINE_CHARS) {
        line[index] = c;
        line[index + 1u] = '\0';
    }
}

/* append_text: appends a null-terminated string to line. */
static void append_text(char *line, const char *text)
{
    while (*text != '\0') {
        append_char(line, *text);
        text++;
    }
}

/* append_uint: appends value as unsigned decimal text to line. */
static void append_uint(char *line, uint32_t value)
{
    char digits[10];
    uint8_t count;

    if (value == 0u) {
        append_char(line, '0');
        return;
    }

    count = 0u;
    while ((value > 0u) && (count < (uint8_t)sizeof(digits))) {
        digits[count] = (char)('0' + (char)(value % 10u));
        value /= 10u;
        count++;
    }

    while (count > 0u) {
        count--;
        append_char(line, digits[count]);
    }
}

void display_model_build(const DisplayModelInput *input, DisplayFrame *frame)
{
    clear_line(frame->line1);
    clear_line(frame->line2);
    clear_line(frame->line3);

    append_text(frame->line1, input->mode_text);
    append_text(frame->line1, " T");
    append_uint(frame->line1, input->track_index);
    append_text(frame->line1, " V");
    append_uint(frame->line1, input->volume);
    append_text(frame->line1, " ");
    append_text(frame->line1, input->order_text);

    append_text(frame->line2, "SD:");
    append_text(frame->line2, input->sd_ready != 0u ? "OK" : "NO");
    append_text(frame->line2, " WAV:");
    append_text(frame->line2, input->file_open != 0u ? "OPEN" : "NONE");

    if (input->file_open != 0u) {
        append_uint(frame->line3, input->sample_rate);
        append_text(frame->line3, "Hz ");
        append_uint(frame->line3, input->channels);
        append_text(frame->line3, "ch");
        append_text(frame->line3, " P");
        append_uint(frame->line3, input->progress_percent);
        append_char(frame->line3, '%');
    } else {
        append_text(frame->line3, "Use TF TRACK01.WAV");
    }
}
