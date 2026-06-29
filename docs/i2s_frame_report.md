# I2S Frame Simulation Report

This report mirrors `drivers/i2s_dac.c` for one known stereo frame before probing the real PCM5102A pins.

| Check | Result |
| --- | --- |
| BCK pulses per stereo frame | 64 |
| Left LRCK slot | 0 for bits 0-31 |
| Right LRCK slot | 1 for bits 32-63 |
| I2S MSB delay | first DIN bit in each channel slot is 0 |
| Slot width | 1 delay bit + 16 sample bits + 15 padding bits |

| Channel | Sample | 32-bit DIN slot |
| --- | --- | --- |
| Left | 0x1234 | `00001001000110100000000000000000` |
| Right | 0xFEDC | `01111111011011100000000000000000` |
