"""Validate TF-card WAV assets against the firmware reader contract.

The firmware WAV reader expects RIFF/WAVE, PCM format, 16-bit samples, and one
or two channels. This script parses the RIFF chunks directly instead of relying
only on Python's wave module, so it catches malformed headers that could pass a
looser host-side playback check.
"""

from __future__ import annotations

import argparse
import struct
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "sdcard"


@dataclass(frozen=True)
class WavAsset:
    path: Path
    channels: int
    sample_rate: int
    bits_per_sample: int
    byte_rate: int
    block_align: int
    data_bytes: int

    @property
    def frames(self) -> int:
        return self.data_bytes // self.block_align


def read_u32le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def parse_wav(path: Path) -> WavAsset:
    blob = path.read_bytes()
    if len(blob) < 44:
        raise ValueError(f"{path} is too small for a WAV header")
    if blob[0:4] != b"RIFF" or blob[8:12] != b"WAVE":
        raise ValueError(f"{path} is not RIFF/WAVE")

    offset = 12
    fmt_chunk: bytes | None = None
    data_bytes: int | None = None
    while offset + 8 <= len(blob):
        chunk_id = blob[offset : offset + 4]
        chunk_size = read_u32le(blob, offset + 4)
        data_start = offset + 8
        data_end = data_start + chunk_size
        if data_end > len(blob):
            raise ValueError(f"{path} has truncated chunk {chunk_id!r}")

        if chunk_id == b"fmt ":
            fmt_chunk = blob[data_start:data_end]
        elif chunk_id == b"data":
            data_bytes = chunk_size

        offset = data_end + (chunk_size & 1)

    if fmt_chunk is None:
        raise ValueError(f"{path} missing fmt chunk")
    if data_bytes is None:
        raise ValueError(f"{path} missing data chunk")
    if len(fmt_chunk) < 16:
        raise ValueError(f"{path} fmt chunk is too short")

    audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample = struct.unpack_from(
        "<HHIIHH", fmt_chunk, 0
    )
    if audio_format != 1:
        raise ValueError(f"{path} must use PCM format, got {audio_format}")
    if channels not in (1, 2):
        raise ValueError(f"{path} must be mono or stereo, got {channels} channels")
    if bits_per_sample != 16:
        raise ValueError(f"{path} must be 16-bit PCM, got {bits_per_sample}")
    expected_align = channels * (bits_per_sample // 8)
    if block_align != expected_align:
        raise ValueError(f"{path} block_align {block_align} != {expected_align}")
    expected_byte_rate = sample_rate * block_align
    if byte_rate != expected_byte_rate:
        raise ValueError(f"{path} byte_rate {byte_rate} != {expected_byte_rate}")
    if data_bytes == 0 or data_bytes % block_align != 0:
        raise ValueError(f"{path} data size is invalid: {data_bytes}")

    return WavAsset(
        path=path,
        channels=channels,
        sample_rate=sample_rate,
        bits_per_sample=bits_per_sample,
        byte_rate=byte_rate,
        block_align=block_align,
        data_bytes=data_bytes,
    )


def find_tracks(input_dir: Path) -> list[Path]:
    tracks = sorted(input_dir.glob("TRACK*.WAV"))
    required = [input_dir / f"TRACK{index:02d}.WAV" for index in range(1, 4)]
    missing = [path.name for path in required if not path.exists()]
    if missing:
        raise FileNotFoundError(f"missing generated test WAV files: {', '.join(missing)}")
    return tracks


def render_report(assets: list[WavAsset]) -> str:
    lines = [
        "# TF WAV Asset Check",
        "",
        "| File | Channels | Rate | Bits | Frames | Data bytes |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for asset in assets:
        lines.append(
            f"| {asset.path.name} | {asset.channels} | {asset.sample_rate} | "
            f"{asset.bits_per_sample} | {asset.frames} | {asset.data_bytes} |"
        )
    lines.append("")
    lines.append("All listed files are RIFF/WAVE PCM assets accepted by the firmware WAV reader.")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, help="Directory containing TRACKxx.WAV files")
    parser.add_argument("--report", type=Path, default=None, help="Optional markdown report path")
    args = parser.parse_args()

    assets = [parse_wav(path) for path in find_tracks(args.input)]
    report = render_report(assets)
    if args.report is not None:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(report, encoding="utf-8", newline="\n")

    print("wav asset checks passed")
    for asset in assets:
        print(
            f"{asset.path.name}: {asset.sample_rate}Hz {asset.channels}ch "
            f"{asset.bits_per_sample}-bit frames={asset.frames}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
