# Software Effect Acceptance Report

This report is generated from host-side simulations and source checks. It proves the software-visible effects before physical flashing; real HC-05 pairing, TF-card electrical I/O, DAC output, amplifier output, and EC11 feel still require board testing.

## Bluetooth Command Chain

| Effect | Evidence |
| --- | --- |
| Android command buttons | p, s, r, n, b, +, -, m, o, t, i, e, l, d, ?, k, u, x, w |
| APK source command coverage | 28 buttons verified in docs/android_command_coverage_report.md |
| APK log evidence export | `Share Log` shares text; `Save Log` writes text through Android document picker |
| Firmware command transcript | playing -> status=playing track=1 volume=18 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V18 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> volume=19 -> status=playing track=1 volume=19 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V19 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> volume=20 -> status=playing track=1 volume=20 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V20 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> open TRACK03.WAV -> status=playing track=3 volume=20 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T3 V20 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> open TRACK01.WAV -> status=playing track=1 volume=20 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V20 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> volume=19 -> status=playing track=1 volume=19 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V19 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> mute=on -> status=playing track=1 volume=0 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V0 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> mute=off -> status=playing track=1 volume=19 order=repeat_all rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V19 ALL -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> order=repeat_one -> status=playing track=1 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> display 1:playing T1 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> stop -> status=stopped track=1 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> display 1:stopped T1 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> open TRACK03.WAV -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> replay -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> tone start -> tone done -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> info name=MSP430F5529-BT-WAV version=1.4.1 profile=TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1 -> selftest bt=ok sd=ok file=open dac=test-with-t -> tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=-- -> display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 -> link rx=19 status=14 display=14 bad=0 last=k uptime=1234ms -> input ecw=0 eccw=0 eb=0 elong=0 s1=0 s1l=0 s2=0 s2l=0 s4=0 s4l=0 -> trace count=6 1=bt:order 2=bt:stop 3=bt:track 4=bt:replay 5=bt:tone 6=bt:trace -> pin profile=TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1 -> pin tf cs=P4.0 sck=P3.1 mosi=P3.2 miso=P3.3 -> pin i2s bck=P4.1 lrck=P4.2 din=P4.3 -> pin ec11 a=P2.1 b=P2.2 sw=P2.3 -> pin local s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0 -> pin bt tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict -> pin epaper optional=P6.0-P6.5 default=disabled |
| Final status line | status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 |
| End-to-end demo | 14 APK button steps verified in docs/end_to_end_demo_report.md |
| Offline APK demo | `Demo RX` replays 21 firmware-style lines and is verified in docs/android_offline_demo_report.md |

## Bluetooth UART Diagnostics

| Case | Final state | Transcript tail |
| --- | --- | --- |
| bring-up transcript | mode=paused, track=1, volume=18, order=repeat_all | pin ec11 a=P2.1 b=P2.2 sw=P2.3 -> pin local s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0 -> pin bt tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict -> pin epaper optional=P6.0-P6.5 default=disabled |
| fragmented android stream | mode=playing, track=3, volume=19, order=repeat_one | display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 |
| uppercase aliases | mode=playing, track=1, volume=0, order=repeat_one | display 1:playing T1 V0 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> status=playing track=1 volume=0 order=repeat_one rate=16000Hz channels=2 progress=0 |
| noise and line endings ignored | mode=paused, track=1, volume=18, order=repeat_all | status=paused track=1 volume=18 order=repeat_all rate=16000Hz channels=2 progress=0 |

## Whole-Board Control Scenario

| Input sequence | Final state |
| --- | --- |
| enc:cw, enc:cw, s1, s1, s1:long, s1, s4, bt:n, s2, bt:-, s2:long, enc:cw, enc:cw, enc:cw, bt:t, bt:r, s4:long, enc:long, s1, enc:press, tick:500ms, tick:5s | BoardState(mode='paused', track=2, volume=3, order='repeat_one', led_toggles=1, status_reports=21, display_reports=20, tone_tests=1, input_encoder_cw=5, input_encoder_ccw=0, input_encoder_button=1, input_encoder_long=1, input_s1_short=4, input_s1_long=1, input_s2_short=1, input_s2_long=1, input_s4_short=1, input_s4_long=1, trace=('bt:t', 'bt:r', 's4:long', 'enc:long', 's1:short', 'enc:sw')) |

## Local Button Timing

| Case | Expected event | Actual event |
| --- | --- | --- |
| S1 short press | play_pause | play_pause |
| S1 long press | stop | stop |
| S2 bounce then short | previous | previous |

## EC11 Encoder Signal Decoding

| Case | Expected event | Actual event |
| --- | --- | --- |
| EC11 clockwise detent | cw | cw |
| EC11 counter-clockwise detent | ccw | ccw |
| EC11 back-and-forth jitter | none | none |
| EC11 switch short press | button | button |
| EC11 switch long press | button_long | button_long |

## Status LED Patterns

| Mode | Expected effect | Observed transitions |
| --- | --- | --- |
| playing | fast blink every 200 ms | [(0, 1), (200, 0), (400, 1), (600, 0), (800, 1), (1000, 0)] |
| paused | slow blink every 1000 ms | [(0, 0), (1000, 1), (2000, 0)] |
| stopped | steady on | [(0, 1)] |
| error | repeating double blink | [(0, 1), (150, 0), (300, 1), (450, 0), (1200, 1), (1350, 0), (1500, 1)] |

## Display And APK Parsing

| Effect | Evidence |
| --- | --- |
| Display frame | display 1:playing T1 V18 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V19 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V20 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T3 V20 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V20 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V19 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V0 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V19 ALL / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T1 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:stopped T1 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T3 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T3 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T3 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% / display 1:playing T3 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% |
| Android parser markers | status=, progress=, sd mounted, info name=, selftest , tone start, open TRACK0, display 1:, display 2:, display 3:, link , input , trace , pin , volumeBar, progressBar, boundedInt, volumeBar.setMax(32), progressBar.setMax(100), Demo RX, DEMO_RX_LINES, runOfflineDemo |
| Android parsed dashboard | Mode: playing / Track: 3 / Volume: 19 / Order: repeat_one / Progress: 42% |
| Android parsed visual bars | volume=19/32 progress=42/100 |
| Android parsed health | Health / SD:OK Info:1.4.1 Selftest:bt=ok sd=ok wav=ok i2s=o Tone:done / File:TRACK01.WAV / Error:-- |
| Android parsed display | Display frame / playing T3 V19 ONE / SD:OK WAV:OPEN / 16000Hz 2ch P42% |
| Android parsed track list | Tracks / 1: ok  2: --  3: ok / 4: --  5: --  6: -- / 7: --  8: --  9: -- |
| Android parsed link | Link / RX: 15  Status: 10  Display: 9 / Bad: 0  Last: k  Uptime: 1234ms |
| Android parsed input | Input / EC11 CW:3 CCW:1 SW:2 Long:1 / S1:2/1 S2:1/1 S4:1/1 |
| Android parsed trace | Trace / 1=bt:vol+  2=bt:next  3=bt:prev / 4=bt:order  5=bt:track  6=bt:trace |
| Android parsed wiring | Wiring / Profile: TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1 / TF: cs=P4.0 sck=P3.1 mosi=P3.2 miso=P3.3 / I2S: bck=P4.1 lrck=P4.2 din=P4.3 / EC11: a=P2.1 b=P2.2 sw=P2.3 / Local: s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0 / BT: tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict / E-paper: optional=P6.0-P6.5 default=disabled |
| Android acceptance summary | Acceptance 9/9 / SD:OK Info:OK Selftest:OK Tracks:OK Wiring:OK / Display:OK Status:OK Tone:OK Open:OK |
| Android offline demo | Mode: playing / Track: 3 / Volume: 19 / Order: repeat_one / Progress: 42% / bars=19/32,42/100 / Acceptance 9/9 / SD:OK Info:OK Selftest:OK Tracks:OK Wiring:OK / Display:OK Status:OK Tone:OK Open:OK |
| E-paper preview | tools/epaper_preview_sim.py renders and checks a nonblank 296x128 PGM frame |
| E-paper gallery | playing, paused, stopped, and error previews are generated in docs/epaper_gallery_report.md |

## Audio Stream Simulation

Firmware stream buffer: 192 bytes. Volume scale: 18/32.

| File | Chunks | Output frames | Peak | Final progress |
| --- | --- | --- | --- | --- |
| TRACK01.WAV | 84 | 4000 | -6750..6750 | 100% |
| TRACK02.WAV | 84 | 4000 | -6750..6750 | 100% |
| TRACK03.WAV | 84 | 4000 | -6750..6750 | 100% |

## I2S Frame Simulation

| Effect | Evidence |
| --- | --- |
| PCM5102A frame layout | docs/i2s_frame_report.md verifies 64 BCK pulses, LRCK left/right slots, one-bit MSB delay, and 15 padding bits |

## TF WAV Assets

| File | Channels | Rate | Bits | Frames | Data bytes |
| --- | --- | --- | --- | --- | --- |
| TRACK01.WAV | 2 | 16000 | 16 | 4000 | 16000 |
| TRACK02.WAV | 2 | 16000 | 16 | 4000 | 16000 |
| TRACK03.WAV | 2 | 16000 | 16 | 4000 | 16000 |

## Remaining Physical Checks

- Flash `Debug/mspbluetooth.out` to MSP430F5529.
- Pair Android phone with HC-05 and confirm command/response latency.
- Confirm TF-card mount and real WAV stream from the course wiring.
- Confirm PCM5102A analog output, PAM8403 speaker output, and 3.5mm headphone output.
- Confirm EC11 detent direction, EC11 long-press stop, and S1/S2/S4 long-press timing on the physical buttons.
