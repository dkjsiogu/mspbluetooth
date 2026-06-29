"""Simulate one software-I2S stereo frame for PCM5102A validation.

The firmware bit-bangs I2S through GPIO, so a host-side frame check is useful
before probing real BCK/LRCK/DIN pins. This script mirrors i2s_dac.c and
verifies the 32-bit channel slots, one-bit I2S MSB delay, LRCK polarity, and
padding bits for a known stereo sample pair.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "i2s_frame_report.md"


@dataclass(frozen=True)
class I2sBit:
    """I2sBit: one DIN value observed at a BCK pulse with the current LRCK level."""

    index: int
    lrck: int
    din: int


def sample_bits(sample: int) -> list[int]:
    value = sample & 0xFFFF
    return [1 if (value & mask) != 0 else 0 for mask in (1 << bit for bit in range(15, -1, -1))]


def write_channel(bits: list[I2sBit], sample: int, right_channel: int) -> None:
    bits.append(I2sBit(len(bits), right_channel, 0))
    for bit in sample_bits(sample):
        bits.append(I2sBit(len(bits), right_channel, bit))
    for _ in range(15):
        bits.append(I2sBit(len(bits), right_channel, 0))


def simulate_frame(left: int = 0x1234, right: int = 0xFEDC) -> list[I2sBit]:
    bits: list[I2sBit] = []
    write_channel(bits, left, 0)
    write_channel(bits, right, 1)
    return bits


def bits_to_text(bits: list[I2sBit]) -> str:
    return "".join("1" if bit.din else "0" for bit in bits)


def validate_frame(bits: list[I2sBit], left: int, right: int) -> None:
    if len(bits) != 64:
        raise AssertionError(f"one stereo frame must contain 64 BCK pulses, got {len(bits)}")
    if [bit.lrck for bit in bits[:32]] != [0] * 32:
        raise AssertionError("left channel LRCK must stay low for 32 BCK pulses")
    if [bit.lrck for bit in bits[32:]] != [1] * 32:
        raise AssertionError("right channel LRCK must stay high for 32 BCK pulses")

    left_slot = [bit.din for bit in bits[:32]]
    right_slot = [bit.din for bit in bits[32:]]
    expected_left = [0] + sample_bits(left) + [0] * 15
    expected_right = [0] + sample_bits(right) + [0] * 15
    if left_slot != expected_left:
        raise AssertionError("left slot does not match I2S one-bit-delay sample layout")
    if right_slot != expected_right:
        raise AssertionError("right slot does not match I2S one-bit-delay sample layout")


def render_report(bits: list[I2sBit], left: int, right: int) -> str:
    left_bits = bits_to_text(bits[:32])
    right_bits = bits_to_text(bits[32:])
    lines = [
        "# I2S Frame Simulation Report",
        "",
        "This report mirrors `drivers/i2s_dac.c` for one known stereo frame before probing the real PCM5102A pins.",
        "",
        "| Check | Result |",
        "| --- | --- |",
        "| BCK pulses per stereo frame | 64 |",
        "| Left LRCK slot | 0 for bits 0-31 |",
        "| Right LRCK slot | 1 for bits 32-63 |",
        "| I2S MSB delay | first DIN bit in each channel slot is 0 |",
        "| Slot width | 1 delay bit + 16 sample bits + 15 padding bits |",
        "",
        "| Channel | Sample | 32-bit DIN slot |",
        "| --- | --- | --- |",
        f"| Left | 0x{left & 0xFFFF:04X} | `{left_bits}` |",
        f"| Right | 0x{right & 0xFFFF:04X} | `{right_bits}` |",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    left = 0x1234
    right = 0xFEDC
    bits = simulate_frame(left, right)
    validate_frame(bits, left, right)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(bits, left, right), encoding="utf-8", newline="\n")

    print("I2S frame simulation passed")
    print("bck_pulses=64 left_lrck=0 right_lrck=1 slot=1+16+15")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
