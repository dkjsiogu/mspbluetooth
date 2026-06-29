/*
 * epaper_panel.h
 * Optional low-level e-paper output for the player display model. The default
 * build keeps the panel disabled so TF-card audio, HC-05 Bluetooth, I2S DAC,
 * and EC11 controls remain the main course-design path. When enabled, this
 * driver mirrors the same three-line DisplayFrame used by Bluetooth and APK.
 */
#ifndef EPAPER_PANEL_H
#define EPAPER_PANEL_H

#include <stdint.h>

#include "display_model.h"

/* EPAPER_WIDTH_PIXELS: optional panel width in pixels for 296x128 modules. */
#define EPAPER_WIDTH_PIXELS             296u

/* EPAPER_HEIGHT_PIXELS: optional panel height in pixels for 296x128 modules. */
#define EPAPER_HEIGHT_PIXELS            128u

/* EPAPER_ROW_BYTES: bytes per scan row, with 8 horizontal pixels per byte. */
#define EPAPER_ROW_BYTES                (EPAPER_WIDTH_PIXELS / 8u)

/* EPAPER_RAM_BYTES: full black/white image payload sent to controller RAM. */
#define EPAPER_RAM_BYTES                (EPAPER_ROW_BYTES * EPAPER_HEIGHT_PIXELS)

/* epaper_panel_init: configures optional GPIO and panel controller state. */
void epaper_panel_init(void);

/* epaper_panel_enabled: returns nonzero when the optional panel is compiled in. */
uint8_t epaper_panel_enabled(void);

/*
 * epaper_panel_show_frame: refreshes the optional panel with one display frame.
 * frame: three-line display model already mirrored over Bluetooth and Android.
 */
void epaper_panel_show_frame(const DisplayFrame *frame);

#endif
