"""Check PCM5102A I2S logic-analyzer captures.

The firmware bit-bangs BCK/LRCK/DIN, so a real board can be validated with a
cheap logic analyzer before debugging the amplifier or speaker. With no input
file this script generates a deterministic CSV sample and validates it. With
`--input capture.csv`, it parses a Saleae-style or generic CSV containing
`time`, `bck`, `lrck`, and `din` columns.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path

from i2s_frame_sim import I2sBit, sample_bits, simulate_frame


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SAMPLE = ROOT / "dist" / "verification" / "i2s_capture_sample.csv"
DEFAULT_REPORT = ROOT / "docs" / "i2s_capture_report.md"


@dataclass(frozen=True)
class LogicSample:
    """LogicSample: one parsed analyzer row after normalizing signal levels."""

    time_text: str
    bck: int
    lrck: int
    din: int


@dataclass(frozen=True)
class CaptureFrame:
    """CaptureFrame: one 64-edge stereo frame extracted from a capture."""

    start_edge: int
    bits: list[I2sBit]
    first_lrck: int


def logic_value(raw: str) -> int:
    """Converts common CSV logic values into 0 or 1."""

    value = raw.strip().lower()
    if value in ("1", "high", "true", "h"):
        return 1
    if value in ("0", "low", "false", "l"):
        return 0
    raise ValueError(f"unsupported logic value: {raw!r}")


def header_index(fieldnames: list[str], name: str) -> str:
    """Finds a case-insensitive CSV column name."""

    for field in fieldnames:
        if field.strip().lower() == name:
            return field
    raise ValueError(f"missing CSV column {name!r}; expected time,bck,lrck,din")


def read_capture(path: Path) -> list[LogicSample]:
    """Reads a logic-analyzer CSV into normalized samples."""

    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise ValueError("CSV capture has no header row")

        time_key = header_index(reader.fieldnames, "time")
        bck_key = header_index(reader.fieldnames, "bck")
        lrck_key = header_index(reader.fieldnames, "lrck")
        din_key = header_index(reader.fieldnames, "din")

        samples: list[LogicSample] = []
        for row in reader:
            samples.append(
                LogicSample(
                    time_text=row[time_key].strip(),
                    bck=logic_value(row[bck_key]),
                    lrck=logic_value(row[lrck_key]),
                    din=logic_value(row[din_key]),
                )
            )
    if len(samples) < 2:
        raise ValueError("CSV capture must contain at least two logic samples")
    return samples


def write_sample_capture(path: Path, bits: list[I2sBit]) -> None:
    """Writes a deterministic logic capture CSV for no-hardware verification."""

    path.parent.mkdir(parents=True, exist_ok=True)
    tick = 0
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["time", "bck", "lrck", "din"])
        writer.writerow([tick, 0, bits[0].lrck, 0])
        for bit in bits:
            tick += 1
            writer.writerow([tick, 0, bit.lrck, bit.din])
            tick += 1
            writer.writerow([tick, 1, bit.lrck, bit.din])
            tick += 1
            writer.writerow([tick, 0, bit.lrck, bit.din])


def rising_edge_bits(samples: list[LogicSample]) -> list[I2sBit]:
    """Samples LRCK and DIN on each BCK rising edge."""

    edges: list[I2sBit] = []
    previous = samples[0]
    for sample in samples[1:]:
        if previous.bck == 0 and sample.bck == 1:
            edges.append(I2sBit(len(edges), sample.lrck, sample.din))
        previous = sample
    return edges


def find_frame(edges: list[I2sBit]) -> CaptureFrame:
    """Finds the first 64-edge I2S frame with two stable 32-bit LRCK slots."""

    for start in range(0, max(0, len(edges) - 63)):
        window = edges[start : start + 64]
        first_lrck = window[0].lrck
        second_lrck = 1 - first_lrck
        if all(bit.lrck == first_lrck for bit in window[:32]) and all(bit.lrck == second_lrck for bit in window[32:]):
            return CaptureFrame(start_edge=start, bits=window, first_lrck=first_lrck)
    raise AssertionError(f"no 64-edge I2S frame found in {len(edges)} BCK rising edges")


def expected_slot(sample: int) -> list[int]:
    """Returns the 32 DIN bits expected for one 16-bit sample in an I2S slot."""

    return [0] + sample_bits(sample) + [0] * 15


def validate_frame(frame: CaptureFrame, expected_left: int | None, expected_right: int | None) -> None:
    """Validates slot structure and optional known sample values."""

    if len(frame.bits) != 64:
        raise AssertionError(f"frame must contain 64 BCK rising edges, got {len(frame.bits)}")
    if frame.first_lrck != 0:
        raise AssertionError("capture starts on a right-channel slot; move trigger earlier or capture a low-LRCK slot first")

    left_slot = [bit.din for bit in frame.bits[:32]]
    right_slot = [bit.din for bit in frame.bits[32:]]
    if expected_left is not None and left_slot != expected_slot(expected_left):
        raise AssertionError("left DIN slot does not match the expected I2S sample")
    if expected_right is not None and right_slot != expected_slot(expected_right):
        raise AssertionError("right DIN slot does not match the expected I2S sample")


def bits_text(bits: list[I2sBit]) -> str:
    """Formats DIN bits for reports."""

    return "".join("1" if bit.din else "0" for bit in bits)


def row(columns: list[object]) -> str:
    """Formats one Markdown table row."""

    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def render_report(source: Path, samples: list[LogicSample], edges: list[I2sBit], frame: CaptureFrame) -> str:
    """Renders a Markdown report for classroom evidence."""

    return "\n".join(
        [
            "# I2S Capture Check Report",
            "",
            "Generated by `tools/i2s_capture_check.py`. It validates a logic-analyzer CSV for the PCM5102A software-I2S pins. This does not prove analog audio quality, but it proves the BCK/LRCK/DIN digital bus shape expected by the DAC.",
            "",
            row(["Item", "Evidence"]),
            row(["---", "---"]),
            row(["Capture source", f"`{source}`"]),
            row(["Logic rows", len(samples)]),
            row(["BCK rising edges", len(edges)]),
            row(["Detected frame", f"edges {frame.start_edge}..{frame.start_edge + 63}"]),
            row(["LRCK layout", "32 low edges then 32 high edges"]),
            row(["Left DIN slot", "`" + bits_text(frame.bits[:32]) + "`"]),
            row(["Right DIN slot", "`" + bits_text(frame.bits[32:]) + "`"]),
            "",
            "## Real Capture Procedure",
            "",
            "1. Connect logic analyzer channels to `P4.1=BCK`, `P4.2=LRCK`, and `P4.3=DIN`.",
            "2. Send Bluetooth command `t` from the APK or serial assistant to generate a DAC test tone.",
            "3. Export CSV with columns named `time,bck,lrck,din`.",
            "4. Run `python tools\\i2s_capture_check.py --input path\\to\\capture.csv --left none --right none` for structural checks, or pass known sample values when the captured frame is deterministic.",
            "",
        ]
    )


def run_check(input_path: Path | None, sample_path: Path, expected_left: int | None, expected_right: int | None) -> tuple[Path, list[LogicSample], list[I2sBit], CaptureFrame]:
    """Runs sample generation or real-capture validation."""

    if input_path is None:
        bits = simulate_frame(expected_left or 0x1234, expected_right or 0xFEDC)
        write_sample_capture(sample_path, bits)
        source = sample_path
    else:
        source = input_path

    samples = read_capture(source)
    edges = rising_edge_bits(samples)
    frame = find_frame(edges)
    validate_frame(frame, expected_left, expected_right)
    return source, samples, edges, frame


def parse_optional_hex(raw: str | None) -> int | None:
    """Parses an optional hex or decimal sample argument."""

    if raw is None:
        return None
    return int(raw, 0) & 0xFFFF


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="Logic-analyzer CSV capture with time,bck,lrck,din columns")
    parser.add_argument("--sample-out", type=Path, default=DEFAULT_SAMPLE, help="Generated sample CSV path")
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    parser.add_argument("--left", default="0x1234", help="Expected left sample for sample mode or known capture; use none to skip")
    parser.add_argument("--right", default="0xFEDC", help="Expected right sample for sample mode or known capture; use none to skip")
    args = parser.parse_args()

    expected_left = None if str(args.left).lower() == "none" else parse_optional_hex(args.left)
    expected_right = None if str(args.right).lower() == "none" else parse_optional_hex(args.right)
    if args.input is not None and (expected_left is not None or expected_right is not None):
        print("checking capture against explicit sample values; pass --left none --right none for structural-only checks")

    source, samples, edges, frame = run_check(args.input, args.sample_out, expected_left, expected_right)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(source, samples, edges, frame), encoding="utf-8", newline="\n")

    print("I2S capture check passed")
    print(f"source={source}")
    print(f"logic_rows={len(samples)} bck_edges={len(edges)} frame_start={frame.start_edge}")
    print(f"I2S capture report generated: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
