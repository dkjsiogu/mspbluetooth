"""Generate small WAV files for TF-card playback tests.

The firmware expects TRACK01.WAV through TRACK09.WAV at the TF-card root.
This script creates deterministic 16-bit PCM stereo files that can validate
FatFs, WAV parsing, I2S output, the amplifier, and the APK transport controls
without needing external audio assets.
"""

from __future__ import annotations

import argparse
import math
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "sdcard"
SAMPLE_RATE = 16_000
AMPLITUDE = 12_000


def write_tone(path: Path, frequency: float, seconds: float) -> None:
    frame_count = int(SAMPLE_RATE * seconds)
    path.parent.mkdir(parents=True, exist_ok=True)

    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)

        for index in range(frame_count):
            angle = 2.0 * math.pi * frequency * (index / SAMPLE_RATE)
            sample = int(math.sin(angle) * AMPLITUDE)
            data = sample.to_bytes(2, "little", signed=True)
            wav.writeframesraw(data + data)
        wav.writeframes(b"")


def verify_wav(path: Path) -> None:
    with wave.open(str(path), "rb") as wav:
        if wav.getnchannels() != 2:
            raise ValueError(f"{path} must be stereo")
        if wav.getsampwidth() != 2:
            raise ValueError(f"{path} must be 16-bit PCM")
        if wav.getframerate() != SAMPLE_RATE:
            raise ValueError(f"{path} sample rate mismatch")
        if wav.getnframes() <= 0:
            raise ValueError(f"{path} has no audio frames")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--seconds", type=float, default=1.5)
    args = parser.parse_args()

    tracks = [
        (1, 440.0),
        (2, 660.0),
        (3, 880.0),
    ]

    for track, frequency in tracks:
        path = args.output / f"TRACK{track:02d}.WAV"
        write_tone(path, frequency, args.seconds)
        verify_wav(path)
        print(f"wrote {path} {frequency:.0f}Hz")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

