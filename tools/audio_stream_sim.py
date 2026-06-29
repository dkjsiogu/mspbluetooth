"""Simulate the firmware WAV-to-I2S sample path on host.

This checks the software-visible audio effect before real hardware is flashed:
generated TF-card WAV files are read in the same chunk size used by the MSP430
player, converted from 16-bit PCM frames, volume-scaled like i2s_dac.c, and
validated as non-silent stereo output.
"""

from __future__ import annotations

import argparse
import re
import struct
import wave
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "sdcard"
DEFAULT_REPORT = ROOT / "docs" / "audio_stream_report.md"
CONFIG_PATH = ROOT / "drivers" / "platform_config.h"


@dataclass(frozen=True)
class AudioConfig:
    """AudioConfig: firmware constants that shape host-side stream simulation."""

    buffer_bytes: int
    default_volume: int
    volume_max: int


@dataclass(frozen=True)
class AudioStreamResult:
    """AudioStreamResult: measured output characteristics for one WAV asset."""

    file_name: str
    sample_rate: int
    channels: int
    frames: int
    chunks: int
    output_frames: int
    min_sample: int
    max_sample: int
    nonzero_frames: int
    final_progress: int


def parse_define_uint(text: str, name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+([0-9]+)u?(?:L|UL)?\b", text, re.M)
    if not match:
        raise ValueError(f"missing integer define: {name}")
    return int(match.group(1))


def load_audio_config() -> AudioConfig:
    text = CONFIG_PATH.read_text(encoding="utf-8")
    return AudioConfig(
        buffer_bytes=parse_define_uint(text, "PLAYER_AUDIO_BUFFER_BYTES"),
        default_volume=parse_define_uint(text, "PLAYER_DEFAULT_VOLUME"),
        volume_max=parse_define_uint(text, "PLAYER_VOLUME_MAX"),
    )


def scale_sample(sample: int, volume: int, volume_max: int) -> int:
    scaled = sample * volume
    if scaled >= 0:
        scaled //= volume_max
    else:
        scaled = -((-scaled) // volume_max)
    if scaled > 32767:
        return 32767
    if scaled < -32768:
        return -32768
    return scaled


def progress_percent(data_bytes: int, remaining_bytes: int) -> int:
    if data_bytes == 0 or remaining_bytes >= data_bytes:
        return 0
    one_percent_bytes = data_bytes // 100
    if one_percent_bytes == 0:
        return 100 if remaining_bytes == 0 else 0
    percent = (data_bytes - remaining_bytes) // one_percent_bytes
    return min(percent, 100)


def simulate_track(path: Path, config: AudioConfig) -> AudioStreamResult:
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        sample_rate = wav.getframerate()
        frames = wav.getnframes()
        if sample_width != 2:
            raise ValueError(f"{path} must be 16-bit PCM")
        if channels not in (1, 2):
            raise ValueError(f"{path} must be mono or stereo")

        bytes_per_frame = channels * sample_width
        data_bytes = frames * bytes_per_frame
        remaining = data_bytes
        output_frames = 0
        nonzero_frames = 0
        chunks = 0
        min_sample = 32767
        max_sample = -32768

        while remaining > 0:
            request_bytes = min(config.buffer_bytes, remaining)
            request_bytes -= request_bytes % bytes_per_frame
            if request_bytes == 0:
                break

            raw = wav.readframes(request_bytes // bytes_per_frame)
            if not raw:
                break
            chunks += 1
            remaining -= len(raw)

            offset = 0
            while offset + bytes_per_frame <= len(raw):
                left = struct.unpack_from("<h", raw, offset)[0]
                right = struct.unpack_from("<h", raw, offset + 2)[0] if channels == 2 else left
                left = scale_sample(left, config.default_volume, config.volume_max)
                right = scale_sample(right, config.default_volume, config.volume_max)
                if channels == 2 and left != right:
                    raise AssertionError(f"{path.name} expected matched generated stereo samples")
                if left != 0 or right != 0:
                    nonzero_frames += 1
                min_sample = min(min_sample, left, right)
                max_sample = max(max_sample, left, right)
                output_frames += 1
                offset += bytes_per_frame

        final_progress = progress_percent(data_bytes, remaining)
        result = AudioStreamResult(
            file_name=path.name,
            sample_rate=sample_rate,
            channels=channels,
            frames=frames,
            chunks=chunks,
            output_frames=output_frames,
            min_sample=min_sample,
            max_sample=max_sample,
            nonzero_frames=nonzero_frames,
            final_progress=final_progress,
        )
        validate_result(result)
        return result


def validate_result(result: AudioStreamResult) -> None:
    if result.output_frames != result.frames:
        raise AssertionError(f"{result.file_name}: output frame count mismatch")
    if result.chunks <= 0:
        raise AssertionError(f"{result.file_name}: no stream chunks were processed")
    if result.nonzero_frames <= result.frames // 2:
        raise AssertionError(f"{result.file_name}: stream looks silent")
    if result.max_sample < 5000 or result.min_sample > -5000:
        raise AssertionError(f"{result.file_name}: scaled waveform peak is too small")
    if result.final_progress != 100:
        raise AssertionError(f"{result.file_name}: final progress should be 100")


def find_tracks(input_dir: Path) -> list[Path]:
    tracks = [input_dir / f"TRACK{index:02d}.WAV" for index in range(1, 4)]
    missing = [path.name for path in tracks if not path.exists()]
    if missing:
        raise FileNotFoundError(f"missing generated test WAV files: {', '.join(missing)}")
    return tracks


def run_audio_stream_simulation(input_dir: Path = DEFAULT_INPUT) -> tuple[AudioConfig, list[AudioStreamResult]]:
    config = load_audio_config()
    return config, [simulate_track(path, config) for path in find_tracks(input_dir)]


def row(columns: list[object]) -> str:
    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def render_report(config: AudioConfig, results: list[AudioStreamResult]) -> str:
    lines = [
        "# Audio Stream Simulation Report",
        "",
        f"Firmware stream buffer: {config.buffer_bytes} bytes. Volume scale: {config.default_volume}/{config.volume_max}.",
        "",
        row(["File", "Rate", "Channels", "Frames", "Chunks", "Output frames", "Min", "Max", "Nonzero frames", "Final progress"]),
        row(["---", "---", "---", "---", "---", "---", "---", "---", "---", "---"]),
    ]
    for result in results:
        lines.append(
            row(
                [
                    result.file_name,
                    result.sample_rate,
                    result.channels,
                    result.frames,
                    result.chunks,
                    result.output_frames,
                    result.min_sample,
                    result.max_sample,
                    result.nonzero_frames,
                    f"{result.final_progress}%",
                ]
            )
        )
    lines.append("")
    lines.append("All tracks produced non-silent volume-scaled stereo samples and reached 100% simulated progress.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, help="Directory containing TRACK01-03.WAV")
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    config, results = run_audio_stream_simulation(args.input)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(config, results), encoding="utf-8", newline="\n")

    print("audio stream simulation passed")
    for result in results:
        print(
            f"{result.file_name}: chunks={result.chunks} frames={result.output_frames} "
            f"peak=[{result.min_sample},{result.max_sample}] progress={result.final_progress}%"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
