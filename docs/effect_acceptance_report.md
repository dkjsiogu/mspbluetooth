# Software Effect Acceptance Report

This report is generated from host-side simulations and source checks. It proves the software-visible effects before physical flashing; real HC-05 pairing, TF-card electrical I/O, DAC output, amplifier output, and EC11 feel still require board testing.

## Bluetooth Command Chain

| Effect | Evidence |
| --- | --- |
| Android command buttons | p, s, r, n, b, +, -, m, o, t, i, e, l, d, ? |
| Firmware command transcript | playing -> volume=19 -> volume=20 -> next track=2 -> previous track=1 -> volume=19 -> mute=on -> mute=off -> order=repeat_one -> stop -> open TRACK03.WAV -> replay -> tone start -> tone done -> info name=MSP430F5529-BT-WAV version=1.4.0 -> selftest bt=ok sd=ok file=open dac=test-with-t -> tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=-- -> display 1:playing T3 V19 ONE -> display 2:SD:OK WAV:OPEN -> display 3:16000Hz 2ch P0% -> status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 |
| Final status line | status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0 |

## Whole-Board Control Scenario

| Input sequence | Final state |
| --- | --- |
| enc:cw, enc:cw, s1, s1, s1:long, s1, s4, bt:n, s2, bt:-, s2:long, enc:cw, enc:cw, enc:cw, bt:t, bt:r, s4:long, enc:press, tick:500ms, tick:5s | BoardState(mode='paused', track=2, volume=3, order='repeat_one', led_toggles=1, status_reports=1, tone_tests=1) |

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
| EC11 switch debounce | button | button |

## Display And APK Parsing

| Effect | Evidence |
| --- | --- |
| Display frame | display 1:playing T3 V19 ONE / display 2:SD:OK WAV:OPEN / display 3:16000Hz 2ch P0% |
| Android parser markers | status=, progress=, display 1:, display 2:, display 3: |
| Android parsed dashboard | Mode: playing / Track: 3 / Volume: 19 / Order: repeat_one / Progress: 42% |
| Android parsed display | Display frame / playing T3 V19 ONE / SD:OK WAV:OPEN / 16000Hz 2ch P42% |
| Android parsed track list | Tracks / 1: ok  2: --  3: ok / 4: --  5: --  6: -- / 7: --  8: --  9: -- |
| E-paper preview | tools/epaper_preview_sim.py renders and checks a nonblank 296x128 PGM frame |
| E-paper gallery | playing, paused, stopped, and error previews are generated in docs/epaper_gallery_report.md |

## Audio Stream Simulation

Firmware stream buffer: 192 bytes. Volume scale: 18/32.

| File | Chunks | Output frames | Peak | Final progress |
| --- | --- | --- | --- | --- |
| TRACK01.WAV | 84 | 4000 | -6750..6750 | 100% |
| TRACK02.WAV | 84 | 4000 | -6750..6750 | 100% |
| TRACK03.WAV | 84 | 4000 | -6750..6750 | 100% |

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
- Confirm EC11 detent direction and S1/S2/S4 long-press timing on the physical buttons.
