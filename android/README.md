# Android Bluetooth Controller

This native Java APK controls the MSP430F5529 Bluetooth WAV player through an
HC-05 classic Bluetooth SPP link. Pair the phone with HC-05 in Android system
settings first, then open the app, select the paired device, and connect.

## Supported Commands

- `p`: play/pause
- `s`: stop
- `b` / `n`: previous/next track
- `+` / `-`: volume up/down
- `r`: replay current track
- `m`: mute/restore volume
- `o`: cycle playback order
- `t`: DAC test tone
- `i`: firmware info
- `e`: self-test summary
- `l`: TRACK01..TRACK09 scan
- `d`: three-line display frame
- `k`: Bluetooth link counters
- `u`: EC11 and local-button counters
- `w`: active wiring diagnostics
- `1`..`9`: direct track selection
- `?`: current status

## Parsed Panels

- `status=...`: dashboard mode, track, volume, order, progress, and visual
  volume/progress bars
- `sd mounted` / `info name=...` / `selftest ...` / `tone ...` /
  `open TRACK...` / `error: ...`: Health panel storage, firmware, self-test,
  audio-test, file-open, and error evidence
- `display 1/2/3:...`: three-line display frame for the e-paper mirror effect
- `tracks ...`: track availability list
- `link ...`: RX/status/display/bad-command/last-command/uptime counters
- `input ...`: EC11 and S1/S2/S4 short/long event counters
- `pin ...`: TF, I2S, EC11, local-button, HC-05, and optional e-paper wiring

`Demo RX` injects a built-in firmware-style transcript into the same parser, so
these panels can be visually checked on a phone before HC-05 pairing.

## Build

```powershell
cd E:\code\ccs\mspbluetooth\android
..\.tools\gradle\bin\gradle.bat assembleDebug
```

Preferred repository-level verifier:

```powershell
cd E:\code\ccs\mspbluetooth
powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1
```

APK output:

```text
android\app\build\outputs\apk\debug\app-debug.apk
```

## Acceptance Button

`Run Acceptance` sends:

```text
h i e l d ? t 1 p + n b o 3 k u w
```

The app logs every command with `TX>` markers, parses firmware responses into
the dashboard, visual volume/progress bars, Health, display-frame, track-list, Link, Input, Wiring, and
`Acceptance X/9` panels, then lets the operator save evidence through `Save Log`
or export it through `Share Log`.

`Demo RX` is the no-hardware visual check for the same panels. It does not send
Bluetooth data; it feeds representative RX lines into the APK parser.

The saved phone log can be checked on the PC:

```powershell
python tools\serial_acceptance_check.py --input path\to\phone_log.txt
python tools\android_acceptance_log_sim.py
python tools\android_offline_demo_sim.py
```
