"""Aggregate board-level acceptance evidence into one report.

This script ties together the real-world evidence that matters after flashing:
HC-05 serial/APK logs, PCM5102A I2S logic captures, and generated deliverables.
With no inputs it uses deterministic sample evidence so CI can verify the
checker itself. With real input files it becomes the final classroom evidence
gate before claiming board-level completion.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from i2s_capture_check import run_check as run_i2s_check
from serial_acceptance_check import run_check as run_serial_check


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "hardware_evidence_report.md"
DEFAULT_SERIAL_SAMPLE = ROOT / "dist" / "verification" / "hardware_evidence_serial_sample.txt"
DEFAULT_I2S_SAMPLE = ROOT / "dist" / "verification" / "hardware_evidence_i2s_sample.csv"


@dataclass(frozen=True)
class EvidenceItem:
    """EvidenceItem: one report row describing a verifiable acceptance artifact."""

    name: str
    source: str
    result: str
    note: str


def row(columns: list[object]) -> str:
    """Formats one Markdown table row."""

    return "| " + " | ".join(str(column).replace("|", "\\|").replace("\n", " / ") for column in columns) + " |"


def artifact_status(path: Path, label: str) -> EvidenceItem:
    """Reports whether an expected local deliverable exists."""

    if path.exists() and path.is_file() and path.stat().st_size > 0:
        return EvidenceItem(label, str(path), "present", f"{path.stat().st_size} bytes")
    return EvidenceItem(label, str(path), "missing", "build or package step has not produced this artifact")


def first_artifact_status(paths: list[Path], label: str) -> EvidenceItem:
    """Reports the first present artifact across repo and delivery layouts."""

    for path in paths:
        if path.exists() and path.is_file() and path.stat().st_size > 0:
            return EvidenceItem(label, str(path), "present", f"{path.stat().st_size} bytes")
    return EvidenceItem(
        label,
        " | ".join(str(path) for path in paths),
        "missing",
        "artifact was not found in either development or delivery package layout",
    )


def serial_evidence(input_path: Path | None, sample_out: Path) -> EvidenceItem:
    """Runs serial transcript checks and returns a report item."""

    source_name, lines, results = run_serial_check(input_path, sample_out if input_path is None else None)
    failed = [result for result in results if not result.passed]
    evidence_kind = "real capture" if input_path is not None else "generated sample"
    if failed:
        return EvidenceItem(
            "HC-05/APK serial transcript",
            source_name,
            "FAIL",
            f"{len(results) - len(failed)}/{len(results)} passed using {evidence_kind}; failed: {', '.join(result.label for result in failed)}",
        )
    return EvidenceItem(
        "HC-05/APK serial transcript",
        source_name,
        "PASS",
        f"{len(results)}/{len(results)} checks passed using {evidence_kind}; lines={len(lines)}",
    )


def i2s_evidence(input_path: Path | None, sample_out: Path) -> EvidenceItem:
    """Runs I2S capture checks and returns a report item."""

    try:
        source, samples, edges, frame = run_i2s_check(
            input_path=input_path,
            sample_path=sample_out,
            expected_left=None if input_path is not None else 0x1234,
            expected_right=None if input_path is not None else 0xFEDC,
        )
    except Exception as exc:  # noqa: BLE001 - report real evidence failures clearly.
        return EvidenceItem("PCM5102A I2S capture", str(input_path or sample_out), "FAIL", str(exc))

    evidence_kind = "real capture" if input_path is not None else "generated sample"
    return EvidenceItem(
        "PCM5102A I2S capture",
        str(source),
        "PASS",
        f"{evidence_kind}; rows={len(samples)}, BCK rising edges={len(edges)}, frame edges {frame.start_edge}..{frame.start_edge + 63}",
    )


def run_evidence_check(serial_log: Path | None, i2s_csv: Path | None, serial_sample: Path, i2s_sample: Path) -> list[EvidenceItem]:
    """Runs all evidence checks and returns report rows."""

    items = [
        first_artifact_status(
            [ROOT / "Debug" / "mspbluetooth.out", ROOT / "firmware" / "mspbluetooth.out"],
            "Firmware build artifact",
        ),
        first_artifact_status(
            [
                ROOT / "android" / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk",
                ROOT / "android" / "mspbluetooth-debug.apk",
            ],
            "Android APK artifact",
        ),
        first_artifact_status(
            [ROOT / "dist" / "mspbluetooth_delivery" / "MANIFEST.txt", ROOT / "MANIFEST.txt"],
            "Delivery package manifest",
        ),
        serial_evidence(serial_log, serial_sample),
        i2s_evidence(i2s_csv, i2s_sample),
    ]
    return items


def render_report(items: list[EvidenceItem], serial_log: Path | None, i2s_csv: Path | None) -> str:
    """Renders the aggregate evidence report."""

    physical_inputs = serial_log is not None and i2s_csv is not None
    lines = [
        "# Hardware Evidence Report",
        "",
        "Generated by `tools/hardware_evidence_check.py`. It aggregates evidence that can be collected after flashing the MSP430F5529 board. Passing with generated samples proves the evidence checker and software models; passing with real `--serial-log` and `--i2s-csv` is the stronger board-level evidence.",
        "",
        row(["Evidence", "Source", "Result", "Note"]),
        row(["---", "---", "---", "---"]),
    ]
    for item in items:
        lines.append(row([item.name, f"`{item.source}`", item.result, item.note]))

    lines.extend(
        [
            "",
            "## Completion Interpretation",
            "",
        ]
    )
    if physical_inputs:
        lines.append("Real serial and I2S capture files were supplied. This is board-level evidence for Bluetooth command/control and PCM5102A digital audio timing. Analog speaker/headphone output and operator-observed EC11/local-button behavior should still be recorded in `docs/test_record.csv`.")
    else:
        lines.append("No real serial/I2S files were supplied. This report is a software/sample evidence check only and must not be used to claim physical hardware completion.")

    lines.extend(
        [
            "",
            "## Real Evidence Command",
            "",
            "```powershell",
            "python tools\\hardware_evidence_check.py --serial-log path\\to\\phone_log.txt --i2s-csv path\\to\\i2s_capture.csv",
            "```",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-log", type=Path, help="Real APK/HC-05 transcript with TX markers")
    parser.add_argument("--i2s-csv", type=Path, help="Real logic-analyzer CSV with time,bck,lrck,din columns")
    parser.add_argument("--serial-sample-out", type=Path, default=DEFAULT_SERIAL_SAMPLE, help="Generated serial sample path")
    parser.add_argument("--i2s-sample-out", type=Path, default=DEFAULT_I2S_SAMPLE, help="Generated I2S sample CSV path")
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    items = run_evidence_check(args.serial_log, args.i2s_csv, args.serial_sample_out, args.i2s_sample_out)
    failed = [item for item in items if item.result == "FAIL"]

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(items, args.serial_log, args.i2s_csv), encoding="utf-8", newline="\n")

    if failed:
        for item in failed:
            print(f"FAIL {item.name}: {item.note}")
        print(f"hardware evidence report generated: {args.report}")
        return 1

    print("hardware evidence check passed")
    for item in items:
        print(f"{item.name}: {item.result}")
    print(f"hardware evidence report generated: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
