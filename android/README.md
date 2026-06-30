# MSP430 Env Android Client

This native Java APK connects to the HC-05 SPP link and controls the
MSP430F5529 environment monitor.

## Panels

- `DATA`: latest temperature, humidity, MQ-2 ADC value, gas level, distance,
  and alarm level parsed from `DATA T=...`.
- `Threshold`: current temperature alarm threshold parsed from `TH=...` or
  `DATA ... TH=...`.
- `History`: `HIST COUNT=...`, `REC ...`, `DUMP END`, `ERR HIST`, and
  `LOG CLEARED`.
- `Wiring`: `pin ...` lines for DHT11, MQ-2, HC-SR04, OLED, HC-05, buzzer,
  and EC11.
- `Trace`: `trace ...`, firmware `info ...`, and `cmd ...` help lines.
- `Log`: full TX/RX transcript for lab evidence.

## Commands

| Button | Command |
| --- | --- |
| Data | `?` |
| Info | `i` |
| Wiring | `w` |
| Trace | `x` |
| T- | `T-` |
| T+ | `T+` |
| Set | `SETT=<value>` |
| Count | `HIST?` |
| First | `HIST 1` |
| Dump | `DUMP` |
| Clear | `CLRLOG` |
| Help | `h` |

`Demo RX` injects firmware-format sample lines without hardware. `Save Log`
exports the current transcript as a text file.

Build and verify:

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1
```
